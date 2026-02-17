#include <algorithm>
#include <chrono>
#include "async_producer.h"
#include "broker_producer.h"
#include "broker_producer_response.h"
#include "transaction_manager.h"
#include "producer_message.h"
#include "consumer.h"
#include "client.h"
#include "sleep_for.h"
#include "undefined.h"
#include "interceptors.h"
#include "partitioner.h"
#include "topic_partition.h"
#include "topic_producer.h"
#include "consumer_message.h"

const int minFunctionalRetryBufferLength = 4 * 1024;
const int minFunctionalRetryBufferBytes = 32 * 1024 * 1024;

AsyncProducer::AsyncProducer(std::shared_ptr<Client> client, std::shared_ptr<TransactionManager> txnmgr)
    : m_client(client), m_conf(client->GetConfig()), m_txnmgr(txnmgr)
{
    LOG_CORE("AsyncProducer initialized with client and transaction manager");
}

void AsyncProducer::init()
{
    m_task << dispatcher();
    m_task << retry_handler();
}
void AsyncProducer::async_close()
{
    m_task << shutdown();
}

coev::awaitable<int> AsyncProducer::close()
{
    async_close();

    co_await m_in_flight.wait();

    if (m_conf->Producer.Return.Successes)
    {
        m_task << [this]() -> coev::awaitable<void>
        {
            do
            {
                std::shared_ptr<ProducerMessage> msg;
                co_await m_replies.get(msg);
                if (msg == nullptr)
                {
                    break;
                }
            } while (true);
        }();
    }

    std::vector<std::shared_ptr<ProducerMessage>> perrs;
    if (m_conf->Producer.Return.Errors)
    {
        LOG_CORE("close collecting error messages");
        do
        {
            std::shared_ptr<ProducerError> event;
            co_await m_replies.get(event);
            if (event == nullptr)
            {
                break;
            }
            perrs.emplace_back(event);
        } while (true);
    }
    else
    {
        LOG_CORE("close discarding error messages");
        std::shared_ptr<ProducerError> dummy;
        co_await m_replies.get(dummy);
    }

    co_return !perrs.empty();
}

bool AsyncProducer::is_transactional()
{
    return m_txnmgr->is_transactional();
}

int AsyncProducer::add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    std::map<std::string, std::vector<PartitionOffsetMetadata>> offsets;
    offsets[msg->m_topic].emplace_back(msg->m_partition, msg->m_offset + 1, 0, metadata);
    return add_offsets_to_txn(offsets, group_id);
}

int AsyncProducer::add_offsets_to_txn(const std::map<std::string, std::vector<PartitionOffsetMetadata>> &offsets, const std::string &group_id)
{
    if (!is_transactional())
    {
        LOG_CORE("add_offsets_to_txn not transactional, returning -1");
        return -1;
    }

    return m_txnmgr->add_offsets_to_txn(offsets, group_id);
}

ProducerTxnStatusFlag AsyncProducer::txn_status()
{
    return m_txnmgr->current_txn_status();
}

int AsyncProducer::begin_txn()
{
    if (!is_transactional())
    {
        LOG_CORE("begin_txn not transactional, returning -1");
        return -1;
    }

    return m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInTransaction), ErrNoError);
}

coev::awaitable<int> AsyncProducer::commit_txn()
{
    if (!is_transactional())
    {
        LOG_CORE("commit_txn not transactional, returning -1");
        co_return -1;
    }

    int err = co_await finish_transaction(true);
    if (err != 0)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> AsyncProducer::abort_txn()
{
    if (!is_transactional())
    {
        LOG_CORE("abort_txn not transactional, returning -1");
        co_return -1;
    }

    int err = co_await finish_transaction(false);
    if (err != 0)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> AsyncProducer::finish_transaction(bool commit)
{
    m_in_flight.add();
    auto msg = std::make_shared<ProducerMessage>();
    if (commit)
    {
        msg->m_flags = static_cast<FlagSet>(FlagSet::Endtxn | FlagSet::Committxn);
        LOG_CORE("finish_transaction created commit transaction message");
    }
    else
    {
        msg->m_flags = static_cast<FlagSet>(FlagSet::Endtxn | FlagSet::Aborttxn);
        LOG_CORE("finish_transaction created abort transaction message");
    }
    m_input.set(msg);
    co_await m_in_flight.wait();

    co_return co_await m_txnmgr->finish_transaction(commit);
}

coev::awaitable<void> AsyncProducer::dispatcher()
{
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        co_await m_input.get(msg);
        assert(msg != nullptr);
        auto shutting_down = false;
        LOG_CORE("dispatcher called with message for topic %s, partition %d", msg->m_topic.c_str(), msg->m_partition);
        if (msg->m_flags & FlagSet::Endtxn)
        {
            int err = 0;
            if (msg->m_flags & FlagSet::Committxn)
            {
                LOG_CORE("dispatcher committing transaction");
                err = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagEndTransaction | ProducerTxnFlagCommittingTransaction), err);
            }
            else
            {
                LOG_CORE("dispatcher aborting transaction");
                err = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagEndTransaction | ProducerTxnFlagAbortingTransaction), err);
            }
            if (err != 0)
            {
                LOG_CORE("dispatcher transaction transition error: %d", err);
            }
            m_in_flight.done();
            continue;
        }

        if (msg->m_flags & FlagSet::Shutdown)
        {
            shutting_down = true;
            m_in_flight.done();
            continue;
        }

        if (msg->m_retries == 0)
        {
            if (shutting_down)
            {
                LOG_CORE("dispatcher shutting down, returning error to message");
                if (m_conf->Producer.Return.Errors)
                {
                    msg->m_err = ErrShuttingDown;
                    m_replies.set(msg);
                }
                continue;
            }
            m_in_flight.add();
            if (is_transactional() && (m_txnmgr->current_txn_status() & ProducerTxnFlagInTransaction) == 0)
            {
                LOG_CORE("dispatcher transaction not ready, returning error");
                return_error(msg, ErrTransactionNotReady);
                continue;
            }
        }

        if (!m_conf->Producer.Interceptors.empty())
        {
            LOG_CORE("dispatcher applying %zu interceptors", m_conf->Producer.Interceptors.size());
            for (auto &interceptor : m_conf->Producer.Interceptors)
            {
                co_await safely_apply_interceptor(msg, interceptor);
            }
        }

        int version = 1;
        if (m_conf->Version.IsAtLeast(V0_11_0_0))
        {
            version = 2;
            LOG_CORE("dispatcher using message version 2");
        }
        else if (!msg->m_headers.empty())
        {
            LOG_CORE("dispatcher headers not supported in old version, returning error");
            return_error(msg, ErrHeaderVersionIsOld);
            continue;
        }
        else
        {
            LOG_CORE("dispatcher using message version 1");
        }

        int size = msg->byte_size(version);
        if (size > m_conf->Producer.MaxMessageBytes)
        {
            LOG_CORE("dispatcher message too large, returning error");
            return_error(msg, ErrMaxMessageBytes);
            continue;
        }

        auto it = m_topic_producer.find(msg->m_topic);
        if (it == m_topic_producer.end())
        {
            LOG_CORE("dispatcher creating new TopicProducer for topic %s", msg->m_topic.c_str());
            it = m_topic_producer.emplace(msg->m_topic, std::make_shared<TopicProducer>(shared_from_this(), msg->m_topic)).first;
            it->second->m_task << it->second->dispatch();
        }
        it->second->m_input.set(msg);
    }
}

coev::awaitable<void> AsyncProducer::retry_handler()
{
    int maxBufferLength = m_conf->Producer.Retry.MaxBufferLength;
    if (0 < maxBufferLength && maxBufferLength < minFunctionalRetryBufferLength)
    {
        LOG_CORE("increasing maxBufferLength from %d to %d", maxBufferLength, minFunctionalRetryBufferLength);
        maxBufferLength = minFunctionalRetryBufferLength;
    }

    int maxBufferBytes = m_conf->Producer.Retry.MaxBufferBytes;
    if (0 < maxBufferBytes && maxBufferBytes < minFunctionalRetryBufferBytes)
    {
        LOG_CORE("increasing maxBufferBytes from %d to %d", maxBufferBytes, minFunctionalRetryBufferBytes);
        maxBufferBytes = minFunctionalRetryBufferBytes;
    }

    int version = 1;
    if (m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        version = 2;
        LOG_CORE("using message version 2");
    }

    int64_t current_byte_size = 0;
    std::queue<std::shared_ptr<ProducerMessage>> buf;
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        co_await m_retries.get(msg);
        if (msg)
        {
            buf.push(msg);
            current_byte_size += msg->byte_size(version);

            if ((maxBufferLength <= 0 || buf.size() < maxBufferLength) && (maxBufferBytes <= 0 || current_byte_size < maxBufferBytes))
            {
                continue;
            }

            auto _msg = buf.front();
            buf.pop();
            if (_msg->m_flags == 0)
            {
                LOG_CORE("updating current_byte_size, subtracting %d bytes", (int)_msg->byte_size(version));
                current_byte_size -= _msg->byte_size(version);
            }
            m_input.set(_msg);
        }
        else
        {
            auto _msg = buf.front();
            buf.pop();
            m_input.set(_msg);
            current_byte_size -= _msg->byte_size(version);
        }
    }
}

coev::awaitable<void> AsyncProducer::shutdown()
{
    m_in_flight.add();
    auto msg = std::make_shared<ProducerMessage>();
    msg->m_flags = Shutdown;
    m_input.set(msg);

    co_await m_in_flight.wait();
    int err = m_client->Close();
    if (err != 0)
    {
        LOG_CORE("shutdown Error closing client: %s", KErrorToString(err));
    }
}

coev::awaitable<void> AsyncProducer::bump_idempotent_producer_epoch()
{
    auto [pid, epoch] = m_txnmgr->get_producer_id();
    if (epoch == std::numeric_limits<int16_t>::max())
    {
        auto _txnmgr = std::make_shared<TransactionManager>(m_conf, m_client);
        if (m_conf->Producer.Idempotent)
        {
            int64_t producer_id;
            int16_t producer_epoch;
            auto err = co_await _txnmgr->init_producer_id(producer_id, producer_epoch);
            if (err == (KError)ErrNoError)
            {
                m_txnmgr = _txnmgr;
            }
        }
        else
        {
            m_txnmgr = _txnmgr;
        }
    }
    else
    {
        m_txnmgr->bump_epoch();
    }
}

int AsyncProducer::maybe_transition_to_error_state(KError err)
{
    int result = 0;
    if (err == ErrClusterAuthorizationFailed || err == ErrProducerFenced ||
        err == ErrUnsupportedVersion || err == ErrTransactionalIDAuthorizationFailed)
    {
        result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
    }
    else if (m_txnmgr->m_coordinator_supports_bumping_epoch && (m_txnmgr->current_txn_status() & ProducerTxnFlagEndTransaction) == 0)
    {
        m_txnmgr->m_epoch_bump_required = true;
        result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    else
    {
        result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    return result;
}

void AsyncProducer::return_error(std::shared_ptr<ProducerMessage> msg, KError err)
{

    if (is_transactional())
    {
        LOG_CORE("return_error transitioning to error state");
        maybe_transition_to_error_state(err);
    }

    if (!is_transactional() && msg->m_has_sequence)
    {
        LOG_CORE("return_error bumping idempotent producer epoch");
        m_task << bump_idempotent_producer_epoch();
    }

    msg->clear();
    if (m_conf->Producer.Return.Errors)
    {
        LOG_CORE("return_error setting error to m_errors channel");
        msg->m_err = err;
        m_replies.set(msg);
    }
    m_in_flight.done();
}

void AsyncProducer::return_errors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err)
{

    for (auto &msg : batch)
    {
        return_error(msg, err);
    }
}

void AsyncProducer::return_successes(const std::vector<std::shared_ptr<ProducerMessage>> &batch)
{

    for (auto &msg : batch)
    {
        if (m_conf->Producer.Return.Successes)
        {
            LOG_CORE("return_successes returning success for message from topic %s, partition %d",
                     msg->m_topic.c_str(), msg->m_partition);
            msg->clear();
            m_replies.set(msg);
        }
        m_in_flight.done();
    }
}

void AsyncProducer::retry_message(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (msg->m_retries >= m_conf->Producer.Retry.Max)
    {
        LOG_CORE("retry_message maximum retries reached, returning error");
        return_error(msg, err);
    }
    else
    {
        msg->m_retries++;
        m_retries.set(msg);
    }
}

void AsyncProducer::retry_messages(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err)
{
    for (auto &msg : batch)
    {
        retry_message(msg, err);
    }
}

coev::awaitable<void> AsyncProducer::retry_batch(const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> partition_set, KError kerr)
{
    for (auto &msg : partition_set->m_messages)
    {
        if (msg->m_retries >= m_conf->Producer.Retry.Max)
        {
            LOG_CORE("retry_batch maximum retries reached for message, returning errors");
            return_errors(partition_set->m_messages, kerr);
            co_return;
        }
        msg->m_retries++;
    }

    std::shared_ptr<Broker> leader;
    int err = co_await m_client->Leader(topic, partition, leader);
    if (err != 0)
    {
        LOG_CORE("retry_batch failed to get leader, error %d, returning errors", err);
        for (auto &msg : partition_set->m_messages)
        {
            return_error(msg, kerr);
        }
        co_return;
    }

    auto _set = std::make_shared<ProduceSet>(shared_from_this());
    _set->m_messages[topic][partition] = partition_set;
    _set->m_buffer_bytes += partition_set->m_buffer_bytes;
    _set->m_buffer_count += partition_set->m_messages.size();

    auto producer = get_broker_producer(leader);
    if (producer)
    {
        producer->m_output.set(_set);
        producer->rollover();
    }
    else
    {
        LOG_CORE("retry_batch error");
    }
}

std::shared_ptr<BrokerProducer> AsyncProducer::get_broker_producer(std::shared_ptr<Broker> &broker)
{
    auto it = m_brokers.find(broker->ID());
    if (it == m_brokers.end())
    {
        LOG_CORE("creating new BrokerProducer for broker %d", broker->ID());
        it = m_brokers.emplace(broker->ID(), std::make_shared<BrokerProducer>(shared_from_this(), broker)).first;
        it->second->init();
    }
    return it->second;
}

coev::awaitable<void> AsyncProducer::abandon_broker_connection(std::shared_ptr<Broker> broker)
{
    bool ok;
    auto _broker = m_brokers[broker->ID()];
    co_await _broker->m_abandoned.get(ok);
    if (ok)
    {
        broker->Close();
        LOG_CORE("removing broker %d from m_brokers", broker->ID());
        m_brokers.erase(broker->ID());
    }
}

coev::awaitable<int> NewAsyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<AsyncProducer> &producer)
{
    std::shared_ptr<Client> client;
    auto err = co_await NewClient(addrs, conf, client);
    if (!client)
    {
        LOG_CORE("client is null, returning ErrClosedClient");
        co_return ErrClosedClient;
    }
    co_return co_await NewAsyncProducer(client, producer);
}
coev::awaitable<int> NewAsyncProducer(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer)
{
    if (client->Closed())
    {
        co_return ErrClosedClient;
    }
    auto txnmgr = std::make_shared<TransactionManager>(client->GetConfig(), client);
    if (client->GetConfig()->Producer.Idempotent)
    {
        int64_t producer_id;
        int16_t producer_epoch;
        auto err = co_await txnmgr->init_producer_id(producer_id, producer_epoch);
        if (err != ErrNoError)
        {
            co_return err;
        }
    }

    producer = std::make_shared<AsyncProducer>(client, txnmgr);
    producer->init();
    co_return ErrNoError;
}
