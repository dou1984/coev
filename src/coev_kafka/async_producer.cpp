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
    : m_client(client), m_conf(client->GetConfig()), m_txnmgr(txnmgr), m_in_flight(0)
{
}

void AsyncProducer::async_close()
{
    co_start << shutdown();
}

coev::awaitable<int> AsyncProducer::close()
{
    async_close();

    if (m_conf->Producer.Return.Successes)
    {
        co_start << [this]() -> coev::awaitable<void>
        {
            std::shared_ptr<ProducerMessage> msg;
            while ((msg = co_await m_successes.get()) != nullptr)
            {
            }
        }();
    }

    std::vector<std::shared_ptr<ProducerError>> pErrs;
    if (m_conf->Producer.Return.Errors)
    {
        std::shared_ptr<ProducerError> event;
        while ((event = co_await m_errors.get()) != nullptr)
        {
            pErrs.push_back(event);
        }
    }
    else
    {
        auto dummy = co_await m_errors.get();
    }

    co_return !pErrs.empty();
}

bool AsyncProducer::is_transactional()
{
    return m_txnmgr->IsTransactional();
}

int AsyncProducer::add_message_to_txn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> offsets;
    auto pom = std::make_shared<PartitionOffsetMetadata>();
    pom->m_partition = msg->m_partition;
    pom->m_offset = msg->m_offset + 1;
    pom->m_metadata = metadata;
    offsets[msg->m_topic].push_back(pom);
    return add_offsets_to_txn(offsets, group_id);
}

int AsyncProducer::add_offsets_to_txn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id)
{
    if (!is_transactional())
    {
        return -1;
    }

    return m_txnmgr->AddOffsetsToTxn(offsets, group_id);
}

ProducerTxnStatusFlag AsyncProducer::txn_status()
{
    return m_txnmgr->CurrentTxnStatus();
}

int AsyncProducer::begin_txn()
{
    if (!is_transactional())
    {
        return -1;
    }

    return m_txnmgr->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInTransaction), ErrNoError);
}

coev::awaitable<int> AsyncProducer::commit_txn()
{
    if (!is_transactional())
    {
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
    m_in_flight++;
    auto msg = std::make_shared<ProducerMessage>();
    if (commit)
    {
        msg->m_flags = static_cast<FlagSet>(FlagSet::Endtxn | FlagSet::Committxn);
    }
    else
    {
        msg->m_flags = static_cast<FlagSet>(FlagSet::Endtxn | FlagSet::Aborttxn);
    }
    co_await dispatcher(msg);

    while (m_in_flight > 0)
    {
        co_await sleep_for(std::chrono::milliseconds(1));
    }

    co_return co_await m_txnmgr->FinishTransaction(commit);
}

coev::awaitable<int> AsyncProducer::dispatcher(std::shared_ptr<ProducerMessage> &msg)
{

    if (msg == nullptr)
    {
        co_return INVALID;
    }
    auto shutting_down = false;
    if (msg->m_flags & FlagSet::Endtxn)
    {
        int err = 0;
        if (msg->m_flags & FlagSet::Committxn)
        {
            err = m_txnmgr->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagEndTransaction | ProducerTxnFlagCommittingTransaction), err);
        }
        else
        {
            err = m_txnmgr->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagEndTransaction | ProducerTxnFlagAbortingTransaction), err);
        }
        if (err != 0)
        {
        }
        m_in_flight--;
        co_return ErrNoError;
    }

    if (msg->m_flags & FlagSet::Shutdown)
    {
        shutting_down = true;
        m_in_flight--;
        co_return ErrNoError;
    }

    if (msg->m_retries == 0)
    {
        if (shutting_down)
        {
            auto pErr = std::make_shared<ProducerError>();
            pErr->m_msg = msg;
            pErr->m_err = ErrShuttingDown;
            if (m_conf->Producer.Return.Errors)
            {
                m_errors = pErr;
            }
            co_return ErrNoError;
        }
        m_in_flight++;

        if (is_transactional() && (m_txnmgr->CurrentTxnStatus() & ProducerTxnFlagInTransaction) == 0)
        {
            return_error(msg, ErrTransactionNotReady);
            co_return ErrNoError;
        }
    }

    for (auto &interceptor : m_conf->Producer.Interceptors)
    {
        co_await SafelyApplyInterceptor(msg, interceptor);
    }

    int version = 1;
    if (m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        version = 2;
    }
    else if (!msg->m_headers.empty())
    {
        return_error(msg, ErrHeaderVersionIsOld);
        co_return INVALID;
    }

    int size = msg->byteSize(version);
    if (size > m_conf->Producer.MaxMessageBytes)
    {
        return_error(msg, ErrMaxMessageBytes);
        co_return ErrNoError;
    }

    auto it = m_topic_producer.find(msg->m_topic);
    if (it != m_topic_producer.end())
    {
        co_await it->second->dispatch(msg);
    }
    else
    {
        auto tp = std::make_shared<TopicProducer>(shared_from_this(), msg->m_topic);
        m_topic_producer[msg->m_topic] = tp;
        co_await tp->dispatch(msg);
    }
    co_return ErrNoError;
}

coev::awaitable<void> AsyncProducer::retry_handler()
{
    int maxBufferLength = m_conf->Producer.Retry.MaxBufferLength;
    if (0 < maxBufferLength && maxBufferLength < minFunctionalRetryBufferLength)
    {
        maxBufferLength = minFunctionalRetryBufferLength;
    }

    int maxBufferBytes = m_conf->Producer.Retry.MaxBufferBytes;
    if (0 < maxBufferBytes && maxBufferBytes < minFunctionalRetryBufferBytes)
    {
        maxBufferBytes = minFunctionalRetryBufferBytes;
    }

    int version = 1;
    if (m_conf->Version.IsAtLeast(V0_11_0_0))
    {
        version = 2;
    }

    int64_t currentByteSize = 0;
    std::queue<std::shared_ptr<ProducerMessage>> buf;
    while (true)
    {
        auto msg = co_await m_retries.get();
        if (msg)
        {
            buf.push(msg);
            currentByteSize += msg->byteSize(version);

            if ((maxBufferLength <= 0 || buf.size() < maxBufferLength) && (maxBufferBytes <= 0 || currentByteSize < maxBufferBytes))
            {
                continue;
            }

            auto msgToHandle = buf.front();
            buf.pop();
            if (msgToHandle->m_flags == 0)
            {
                currentByteSize -= msgToHandle->byteSize(version);
            }
        }
        else
        {
            auto msgToHandle = buf.front();
            buf.pop();
            co_await dispatcher(msgToHandle);
            currentByteSize -= msgToHandle->byteSize(version);
        }
    }
}

coev::awaitable<void> AsyncProducer::shutdown()
{
    m_in_flight++;
    auto shutdown_msg = std::make_shared<ProducerMessage>();
    shutdown_msg->m_flags = Shutdown;
    co_await dispatcher(shutdown_msg);

    while (m_in_flight > 0)
    {
        co_await sleep_for(std::chrono::milliseconds(1));
    }

    int err = m_client->Close();
    if (err != 0)
    {
        LOG_CORE("Error closing client: %s", KErrorToString(err));
    }
}

coev::awaitable<void> AsyncProducer::bump_idempotent_producer_epoch()
{

    auto [pid, epoch] = m_txnmgr->GetProducerID();
    if (epoch == std::numeric_limits<int16_t>::max())
    {
        std::shared_ptr<TransactionManager> new_txnmgr;
        auto err = co_await NewTransactionManager(m_conf, m_client, new_txnmgr);
        if (err == (KError)ErrNoError)
        {
            m_txnmgr = new_txnmgr;
        }
    }
    else
    {
        m_txnmgr->BumpEpoch();
    }
}

int AsyncProducer::maybe_transition_to_error_state(KError err)
{
    if (err == ErrClusterAuthorizationFailed || err == ErrProducerFenced ||
        err == ErrUnsupportedVersion || err == ErrTransactionalIDAuthorizationFailed)
    {
        return m_txnmgr->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
    }
    if (m_txnmgr->m_coordinator_supports_bumping_epoch && (m_txnmgr->CurrentTxnStatus() & ProducerTxnFlagEndTransaction) == 0)
    {
        m_txnmgr->m_epoch_bump_required = true;
    }
    return m_txnmgr->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
}

void AsyncProducer::return_error(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (is_transactional())
    {
        maybe_transition_to_error_state(err);
    }

    if (!is_transactional() && msg->m_has_sequence)
    {
        bump_idempotent_producer_epoch();
    }

    msg->clear();
    auto pErr = std::make_shared<ProducerError>();
    pErr->m_msg = msg;
    pErr->m_err = err;

    if (m_conf->Producer.Return.Errors)
    {
        m_errors.set(pErr);
    }
    m_in_flight--;
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
            msg->clear();
            m_successes.set(msg);
        }
        m_in_flight--;
    }
}

void AsyncProducer::retry_message(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (msg->m_retries >= m_conf->Producer.Retry.Max)
    {
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

coev::awaitable<void> AsyncProducer::retry_batch(const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet, KError kerr)
{
    for (auto &msg : pSet->m_msgs)
    {
        if (msg->m_retries >= m_conf->Producer.Retry.Max)
        {
            return_errors(pSet->m_msgs, kerr);
            co_return;
        }
        msg->m_retries++;
    }

    std::shared_ptr<Broker> leader;
    int err = co_await m_client->Leader(topic, partition, leader);
    if (err != 0)
    {
        for (auto &msg : pSet->m_msgs)
        {
            return_error(msg, kerr);
        }
        co_return;
    }

    auto bp = get_broker_producer(leader);
    auto produce_set = std::make_shared<ProduceSet>(shared_from_this());
    produce_set->m_messages[topic][partition] = pSet;
    produce_set->m_buffer_bytes += pSet->m_buffer_bytes;
    produce_set->m_buffer_count += pSet->m_msgs.size();

    bp->m_output.set(produce_set);
    unref_broker_producer(leader, bp);
}

std::shared_ptr<BrokerProducer> AsyncProducer::get_broker_producer(std::shared_ptr<Broker> &broker)
{
    auto bp = m_brokers[broker];
    if (!bp)
    {
        bp = std::make_shared<BrokerProducer>(shared_from_this(), broker);
        m_brokers[broker] = bp;
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

        if (m_brokers[broker] == bp)
        {
            m_brokers.erase(broker);
        }
    }
}

coev::awaitable<void> AsyncProducer::abandon_broker_connection(std::shared_ptr<Broker> broker)
{
    auto &bc = m_brokers[broker];
    auto ok = co_await bc->m_abandoned.get();
    if (ok)
    {
        co_await broker->Close();
        m_brokers.erase(broker);
    }
}

coev::awaitable<int> NewAsyncProducer(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<AsyncProducer> &producer)
{
    std::shared_ptr<Client> client;
    auto err = co_await NewClient(addrs, conf, client);
    if (!client)
    {
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
    std::shared_ptr<TransactionManager> txnmgr;
    auto err = co_await NewTransactionManager(client->GetConfig(), client, txnmgr);
    if (err != ErrNoError)
    {
        co_return err;
    }

    producer = std::make_shared<AsyncProducer>(client, txnmgr);
    // producer->m_task << producer->dispatcher();
    producer->m_task << producer->retry_handler();
    co_return ErrNoError;
}

coev::awaitable<int> NewAsyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer)
{
    return NewAsyncProducer(client, producer);
}
