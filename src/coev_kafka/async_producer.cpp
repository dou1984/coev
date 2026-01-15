#include <algorithm>
#include <chrono>
#include "async_producer.h"
#include "broker_producer.h"
#include "broker_producer_response.h"
#include "transaction_manager.h"
#include "producer_message.h"
#include "consumer.h"
#include "client.h"
#include "metrics.h"
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
    m_metrics_registry = std::make_shared<metrics::CleanupRegistry>(client->GetConfig()->MetricRegistry);
}

void AsyncProducer::AsyncClose()
{
    co_start << shutdown();
}

coev::awaitable<int> AsyncProducer::Close()
{
    AsyncClose();

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

bool AsyncProducer::IsTransactional()
{
    return m_txnmgr->IsTransactional();
}

int AsyncProducer::AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> offsets;
    auto pom = std::make_shared<PartitionOffsetMetadata>();
    pom->m_partition = msg->m_partition;
    pom->m_offset = msg->m_offset + 1;
    pom->m_metadata = metadata;
    offsets[msg->m_topic].push_back(pom);
    return AddOffsetsToTxn(offsets, group_id);
}

int AsyncProducer::AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id)
{
    std::lock_guard<std::mutex> lock(m_tx_lock);

    if (!IsTransactional())
    {
        return -1;
    }

    return m_txnmgr->AddOffsetsToTxn(offsets, group_id);
}

ProducerTxnStatusFlag AsyncProducer::TxnStatus()
{
    return m_txnmgr->CurrentTxnStatus();
}

int AsyncProducer::BeginTxn()
{
    std::lock_guard<std::mutex> lock(m_tx_lock);

    if (!IsTransactional())
    {
        return -1;
    }

    return m_txnmgr->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInTransaction), ErrNoError);
}

coev::awaitable<int> AsyncProducer::CommitTxn()
{
    std::lock_guard<std::mutex> lock(m_tx_lock);

    if (!IsTransactional())
    {
        co_return -1;
    }

    int err = co_await finishTransaction(true);
    if (err != 0)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> AsyncProducer::AbortTxn()
{
    std::lock_guard<std::mutex> lock(m_tx_lock);

    if (!IsTransactional())
    {
        co_return -1;
    }

    int err = co_await finishTransaction(false);
    if (err != 0)
    {
        co_return err;
    }
    co_return 0;
}

coev::awaitable<int> AsyncProducer::finishTransaction(bool commit)
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
    m_input.set(msg);

    while (m_in_flight > 0)
    {
        co_await sleep_for(std::chrono::milliseconds(1));
    }

    co_return co_await m_txnmgr->FinishTransaction(commit);
}

coev::awaitable<void> AsyncProducer::dispatcher()
{
    std::map<std::string, coev::co_channel<std::shared_ptr<ProducerMessage>>> handlers;
    bool shutting_down = false;

    while (true)
    {
        std::shared_ptr<ProducerMessage> msg = co_await m_input.get();
        if (!msg)
        {
            continue;
        }

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
            continue;
        }

        if (msg->m_flags & FlagSet::Shutdown)
        {
            shutting_down = true;
            m_in_flight--;
            continue;
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
                    m_errors.set(pErr);
                }
                continue;
            }
            m_in_flight++;

            if (IsTransactional() && (m_txnmgr->CurrentTxnStatus() & ProducerTxnFlagInTransaction) == 0)
            {
                returnError(msg, ErrTransactionNotReady);
                continue;
            }
        }

        for (auto &interceptor : m_conf->Producer.Interceptors)
        {
            co_await safelyApplyInterceptor(msg, interceptor);
        }

        int version = 1;
        if (m_conf->Version.IsAtLeast(V0_11_0_0))
        {
            version = 2;
        }
        else if (!msg->m_headers.empty())
        {
            returnError(msg, ErrHeaderVersionIsOld);
            continue;
        }

        int size = msg->ByteSize(version);
        if (size > m_conf->Producer.MaxMessageBytes)
        {
            returnError(msg, ErrMaxMessageBytes);
            continue;
        }

        if (auto it = handlers.find(msg->m_topic); it != handlers.end())
        {
            it->second.set(msg);
        }
        else
        {
            auto tp = std::make_shared<TopicProducer>(shared_from_this(), msg->m_topic);
            m_task << tp->dispatch();
            handlers[msg->m_topic].set(msg);
        }
    }
}

coev::awaitable<void> AsyncProducer::retryHandler()
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
            currentByteSize += msg->ByteSize(version);

            if ((maxBufferLength <= 0 || buf.size() < maxBufferLength) && (maxBufferBytes <= 0 || currentByteSize < maxBufferBytes))
            {
                continue;
            }

            auto msgToHandle = buf.front();
            buf.pop();
            if (msgToHandle->m_flags == 0)
            {
                currentByteSize -= msgToHandle->ByteSize(version);
            }
        }
        else
        {
            auto msgToHandle = buf.front();
            buf.pop();
            m_input.set(msgToHandle);
            currentByteSize -= msgToHandle->ByteSize(version);
        }
    }
}

coev::awaitable<void> AsyncProducer::shutdown()
{
    m_in_flight++;
    auto shutdown_msg = std::make_shared<ProducerMessage>();
    shutdown_msg->m_flags = Shutdown;
    m_input.set(shutdown_msg);

    while (m_in_flight > 0)
    {
        co_await sleep_for(std::chrono::milliseconds(1));
    }

    int err = m_client->Close();
    if (err != 0)
    {
        LOG_CORE("Error closing client: %s", KErrorToString(err));
    }

    m_metrics_registry->UnregisterAll();
}

coev::awaitable<void> AsyncProducer::bumpIdempotentProducerEpoch()
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

int AsyncProducer::maybeTransitionToErrorState(KError err)
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

void AsyncProducer::returnError(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (IsTransactional())
    {
        maybeTransitionToErrorState(err);
    }

    if (!IsTransactional() && msg->m_has_sequence)
    {
        bumpIdempotentProducerEpoch();
    }

    msg->Clear();
    auto pErr = std::make_shared<ProducerError>();
    pErr->m_msg = msg;
    pErr->m_err = err;

    if (m_conf->Producer.Return.Errors)
    {
        m_errors.set(pErr);
    }
    m_in_flight--;
}

void AsyncProducer::returnErrors(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err)
{
    for (auto &msg : batch)
    {
        returnError(msg, err);
    }
}

void AsyncProducer::returnSuccesses(const std::vector<std::shared_ptr<ProducerMessage>> &batch)
{
    for (auto &msg : batch)
    {
        if (m_conf->Producer.Return.Successes)
        {
            msg->Clear();
            m_successes.set(msg);
        }
        m_in_flight--;
    }
}

void AsyncProducer::retryMessage(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (msg->m_retries >= m_conf->Producer.Retry.Max)
    {
        returnError(msg, err);
    }
    else
    {
        msg->m_retries++;
        m_retries.set(msg);
    }
}

void AsyncProducer::retryMessages(const std::vector<std::shared_ptr<ProducerMessage>> &batch, KError err)
{
    for (auto &msg : batch)
    {
        retryMessage(msg, err);
    }
}

coev::awaitable<void> AsyncProducer::retryBatch(const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet, KError kerr)
{
    for (auto &msg : pSet->m_msgs)
    {
        if (msg->m_retries >= m_conf->Producer.Retry.Max)
        {
            returnErrors(pSet->m_msgs, kerr);
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
            returnError(msg, kerr);
        }
        co_return;
    }

    auto bp = getBrokerProducer(leader);
    auto produce_set = std::make_shared<ProduceSet>(shared_from_this());
    produce_set->m_msgs[topic][partition] = pSet;
    produce_set->m_buffer_bytes += pSet->m_buffer_bytes;
    produce_set->m_buffer_count += pSet->m_msgs.size();

    bp->m_output.set(produce_set);
    unrefBrokerProducer(leader, bp);
}

std::shared_ptr<BrokerProducer> AsyncProducer::getBrokerProducer(std::shared_ptr<Broker> &broker)
{
    std::lock_guard<std::mutex> lock(m_broker_lock);
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

void AsyncProducer::unrefBrokerProducer(std::shared_ptr<Broker> broker, std::shared_ptr<BrokerProducer> bp)
{
    std::lock_guard<std::mutex> lock(m_broker_lock);
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

coev::awaitable<void> AsyncProducer::abandonBrokerConnection(std::shared_ptr<Broker> broker)
{
    std::lock_guard<std::mutex> lock(m_broker_lock);

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
    producer->m_task << producer->dispatcher();
    producer->m_task << producer->retryHandler();
    co_return ErrNoError;
}

coev::awaitable<int> NewAsyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer)
{
    return NewAsyncProducer(client, producer);
}
