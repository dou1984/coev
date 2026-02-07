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

void AsyncProducer::async_close()
{
    m_task << shutdown();
}

coev::awaitable<int> AsyncProducer::close()
{
    LOG_CORE("close called");
    async_close();

    // Wait for all in-flight messages to complete, similar to Sarama's inFlight.Wait()
    LOG_CORE("close waiting for all in-flight messages to complete");
    co_await m_in_flight.wait();
    LOG_CORE("close all in-flight messages completed");

    if (m_conf->Producer.Return.Successes)
    {
        LOG_CORE("close waiting for success messages");
        m_task << [this]() -> coev::awaitable<void>
        {
            do
            {
                std::shared_ptr<ProducerMessage> msg;
                co_await m_successes.get(msg);
                if (msg == nullptr)
                {
                    break;
                }
            } while (true);
        }();
    }

    std::vector<std::shared_ptr<ProducerMessage>> pErrs;
    if (m_conf->Producer.Return.Errors)
    {
        LOG_CORE("close collecting error messages");
        do
        {
            std::shared_ptr<ProducerError> event;
            co_await m_errors.get(event);
            if (event == nullptr)
            {
                break;
            }
            pErrs.emplace_back(event);
        } while (true);
    }
    else
    {
        LOG_CORE("close discarding error messages");
        std::shared_ptr<ProducerError> dummy;
        co_await m_errors.get(dummy);
    }

    LOG_CORE("close returning %d errors", (int)pErrs.size());
    co_return !pErrs.empty();
}

bool AsyncProducer::is_transactional()
{
    auto result = m_txnmgr->is_transactional();
    LOG_CORE("is_transactional returning %d", static_cast<int>(result));
    return result;
}

int AsyncProducer::add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    LOG_CORE("add_message_to_txn called for message from topic %s, partition %d, offset %ld",
             msg->m_topic.c_str(), msg->m_partition, msg->m_offset);
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> offsets;
    auto pom = std::make_shared<PartitionOffsetMetadata>();
    pom->m_partition = msg->m_partition;
    pom->m_offset = msg->m_offset + 1;
    pom->m_metadata = metadata;
    offsets[msg->m_topic].push_back(pom);
    int result = add_offsets_to_txn(offsets, group_id);
    LOG_CORE("add_message_to_txn returning %d", result);
    return result;
}

int AsyncProducer::add_offsets_to_txn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id)
{
    LOG_CORE("add_offsets_to_txn called for %zu topics, group_id: %s", offsets.size(), group_id.c_str());
    if (!is_transactional())
    {
        LOG_CORE("add_offsets_to_txn not transactional, returning -1");
        return -1;
    }

    int result = m_txnmgr->add_offsets_to_txn(offsets, group_id);
    LOG_CORE("add_offsets_to_txn returning %d", result);
    return result;
}

ProducerTxnStatusFlag AsyncProducer::txn_status()
{
    auto result = m_txnmgr->current_txn_status();
    LOG_CORE("txn_status returning %d", (int)result);
    return result;
}

int AsyncProducer::begin_txn()
{
    LOG_CORE("begin_txn called");
    if (!is_transactional())
    {
        LOG_CORE("begin_txn not transactional, returning -1");
        return -1;
    }

    int result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInTransaction), ErrNoError);
    LOG_CORE("begin_txn returning %d", result);
    return result;
}

coev::awaitable<int> AsyncProducer::commit_txn()
{
    LOG_CORE("commit_txn called");
    if (!is_transactional())
    {
        LOG_CORE("commit_txn not transactional, returning -1");
        co_return -1;
    }

    int err = co_await finish_transaction(true);
    LOG_CORE("commit_txn finish_transaction returned %d", err);
    if (err != 0)
    {
        co_return err;
    }
    LOG_CORE("commit_txn returning 0");
    co_return 0;
}

coev::awaitable<int> AsyncProducer::abort_txn()
{
    LOG_CORE("abort_txn called");
    if (!is_transactional())
    {
        LOG_CORE("abort_txn not transactional, returning -1");
        co_return -1;
    }

    int err = co_await finish_transaction(false);
    LOG_CORE("abort_txn finish_transaction returned %d", err);
    if (err != 0)
    {
        co_return err;
    }
    LOG_CORE("abort_txn returning 0");
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
    LOG_CORE("finish_transaction calling dispatcher");
    m_input.set(msg);
    co_await m_in_flight.wait();

    int result = co_await m_txnmgr->finish_transaction(commit);
    LOG_CORE("finish_transaction TxnManager::FinishTransaction returned %d", result);
    co_return result;
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
                    m_errors.set(msg);
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
                co_await SafelyApplyInterceptor(msg, interceptor);
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
            m_topic_producer[msg->m_topic] = std::make_shared<TopicProducer>(shared_from_this(), msg->m_topic);
            it = m_topic_producer.find(msg->m_topic);
        }
        LOG_CORE("dispatcher message dispatched successfully");
        it->second->m_input.set(msg);
    }
}

coev::awaitable<void> AsyncProducer::retry_handler()
{
    LOG_CORE("starting");
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

    int64_t currentByteSize = 0;
    std::queue<std::shared_ptr<ProducerMessage>> buf;
    while (true)
    {
        LOG_CORE("waiting for messages from m_retries channel");
        std::shared_ptr<ProducerMessage> msg;
        co_await m_retries.get(msg);
        if (msg)
        {
            LOG_CORE("received retry message for topic %s, partition %d", msg->m_topic.c_str(), msg->m_partition);
            buf.push(msg);
            currentByteSize += msg->byte_size(version);
            LOG_CORE("added message to buffer, buffer size: %zu, currentByteSize: %ld", buf.size(), currentByteSize);

            if ((maxBufferLength <= 0 || buf.size() < maxBufferLength) && (maxBufferBytes <= 0 || currentByteSize < maxBufferBytes))
            {
                continue;
            }

            LOG_CORE("buffer full, processing first message");
            auto msg_to_handle = buf.front();
            buf.pop();
            LOG_CORE("processing message from buffer for topic %s, partition %d", msg_to_handle->m_topic.c_str(), msg_to_handle->m_partition);
            if (msg_to_handle->m_flags == 0)
            {
                LOG_CORE("updating currentByteSize, subtracting %d bytes", (int)msg_to_handle->byte_size(version));
                currentByteSize -= msg_to_handle->byte_size(version);
            }
            LOG_CORE("calling dispatcher for buffered message");
            m_input.set(msg_to_handle);
        }
        else
        {
            auto msg_to_handle = buf.front();
            buf.pop();
            m_input.set(msg_to_handle);
            LOG_CORE("updating currentByteSize, subtracting %d bytes", (int)msg_to_handle->byte_size(version));
            currentByteSize -= msg_to_handle->byte_size(version);
        }
    }
}

coev::awaitable<void> AsyncProducer::shutdown()
{
    LOG_CORE("shutdown starting");
    m_in_flight.add();
    auto msg = std::make_shared<ProducerMessage>();
    msg->m_flags = Shutdown;
    LOG_CORE("shutdown sending shutdown message to dispatcher");
    m_input.set(msg);

    co_await m_in_flight.wait();
    LOG_CORE("shutdown closing client connection");
    int err = m_client->Close();
    if (err != 0)
    {
        LOG_CORE("shutdown Error closing client: %s", KErrorToString(err));
    }
}

coev::awaitable<void> AsyncProducer::bump_idempotent_producer_epoch()
{
    auto [pid, epoch] = m_txnmgr->get_producer_id();
    LOG_CORE("bump_idempotent_producer_epoch current pid: %ld, epoch: %d", pid, epoch);
    if (epoch == std::numeric_limits<int16_t>::max())
    {
        LOG_CORE("bump_idempotent_producer_epoch epoch reached max value, creating new transaction manager");
        auto new_txnmgr = std::make_shared<TransactionManager>(m_conf, m_client);
        if (m_conf->Producer.Idempotent)
        {
            int64_t producer_id;
            int16_t producer_epoch;
            auto err = co_await new_txnmgr->init_producer_id(producer_id, producer_epoch);
            if (err == (KError)ErrNoError)
            {
                m_txnmgr = new_txnmgr;
            }
        }
        else
        {
            m_txnmgr = new_txnmgr;
        }
    }
    else
    {
        LOG_CORE("bump_idempotent_producer_epoch bumping epoch from %d to %d", epoch, epoch + 1);
        m_txnmgr->bump_epoch();
    }
}

int AsyncProducer::maybe_transition_to_error_state(KError err)
{
    LOG_CORE("maybe_transition_to_error_state called with error %d", (int)err);
    int result = 0;
    if (err == ErrClusterAuthorizationFailed || err == ErrProducerFenced ||
        err == ErrUnsupportedVersion || err == ErrTransactionalIDAuthorizationFailed)
    {
        LOG_CORE("maybe_transition_to_error_state transitioning to fatal error state");
        result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
    }
    else if (m_txnmgr->m_coordinator_supports_bumping_epoch && (m_txnmgr->current_txn_status() & ProducerTxnFlagEndTransaction) == 0)
    {
        LOG_CORE("maybe_transition_to_error_state setting epoch_bump_required to true");
        m_txnmgr->m_epoch_bump_required = true;
        LOG_CORE("maybe_transition_to_error_state transitioning to abortable error state");
        result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    else
    {
        LOG_CORE("maybe_transition_to_error_state transitioning to abortable error state");
        result = m_txnmgr->transition_to(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
    }
    LOG_CORE("maybe_transition_to_error_state returning %d", result);
    return result;
}

void AsyncProducer::return_error(std::shared_ptr<ProducerMessage> msg, KError err)
{
    LOG_CORE("return_error called for message from topic %s, partition %d, error %d",
             msg->m_topic.c_str(), msg->m_partition, (int)err);
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
        m_errors.set(msg);
    }
    m_in_flight.done();
}

void AsyncProducer::return_errors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err)
{
    LOG_CORE("return_errors called for batch of %zu messages, error %d", batch.size(), (int)err);
    for (auto &msg : batch)
    {
        return_error(msg, err);
    }
}

void AsyncProducer::return_successes(const std::vector<std::shared_ptr<ProducerMessage>> &batch)
{
    LOG_CORE("return_successes called for batch of %zu messages", batch.size());
    for (auto &msg : batch)
    {
        if (m_conf->Producer.Return.Successes)
        {
            LOG_CORE("return_successes returning success for message from topic %s, partition %d",
                     msg->m_topic.c_str(), msg->m_partition);
            msg->clear();
            m_successes.set(msg);
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

coev::awaitable<void> AsyncProducer::retry_batch(const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pset, KError kerr)
{
    LOG_CORE("retry_batch called for topic %s, partition %d, %zu messages, error %d",
             topic.c_str(), partition, pset->m_messages.size(), (int)kerr);
    for (auto &msg : pset->m_messages)
    {
        if (msg->m_retries >= m_conf->Producer.Retry.Max)
        {
            LOG_CORE("retry_batch maximum retries reached for message, returning errors");
            return_errors(pset->m_messages, kerr);
            co_return;
        }
        LOG_CORE("retry_batch incrementing retry count for message to %d", msg->m_retries + 1);
        msg->m_retries++;
    }

    std::shared_ptr<Broker> leader;
    LOG_CORE("retry_batch getting leader for topic %s, partition %d", topic.c_str(), partition);
    int err = co_await m_client->Leader(topic, partition, leader);
    if (err != 0)
    {
        LOG_CORE("retry_batch failed to get leader, error %d, returning errors", err);
        for (auto &msg : pset->m_messages)
        {
            return_error(msg, kerr);
        }
        co_return;
    }

    LOG_CORE("retry_batch got leader broker %d, getting broker producer", leader->ID());
    auto bp = get_broker_producer(leader);
    auto produce_set = std::make_shared<ProduceSet>(shared_from_this());
    produce_set->m_messages[topic][partition] = pset;
    produce_set->m_buffer_bytes += pset->m_buffer_bytes;
    produce_set->m_buffer_count += pset->m_messages.size();

    LOG_CORE("retry_batch sending produce_set to broker producer, %d messages, %d bytes", produce_set->m_buffer_count, produce_set->m_buffer_bytes);
    bp->m_output.set(produce_set);
    unref_broker_producer(leader, bp);
    LOG_CORE("retry_batch completed");
}

std::shared_ptr<BrokerProducer> AsyncProducer::get_broker_producer(std::shared_ptr<Broker> &broker)
{
    LOG_CORE("called for broker %d", broker->ID());
    auto bp = m_brokers[broker->ID()];
    if (!bp)
    {
        LOG_CORE("creating new BrokerProducer for broker %d", broker->ID());
        bp = std::make_shared<BrokerProducer>(shared_from_this(), broker);
        m_brokers[broker->ID()] = bp;
        m_broker_refs[bp] = 0;
    }

    m_broker_refs[bp]++;
    return bp;
}

void AsyncProducer::unref_broker_producer(std::shared_ptr<Broker> broker, std::shared_ptr<BrokerProducer> bp)
{
    m_broker_refs[bp]--;
    if (m_broker_refs[bp] == 0)
    {
        m_broker_refs.erase(bp);
        if (m_brokers[broker->ID()] == bp)
        {
            m_brokers.erase(broker->ID());
        }
    }
}

coev::awaitable<void> AsyncProducer::abandon_broker_connection(std::shared_ptr<Broker> broker)
{
    bool ok;
    co_await m_brokers[broker->ID()]->m_abandoned.get(ok);
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
    producer->m_task << producer->dispatcher();
    producer->m_task << producer->retry_handler();
    co_return ErrNoError;
}

coev::awaitable<int> NewAsyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer)
{
    return NewAsyncProducer(client, producer);
}
