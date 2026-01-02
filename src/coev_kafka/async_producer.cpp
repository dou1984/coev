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
    : client_(client), conf_(client->GetConfig()), txnmgr_(txnmgr), in_flight_(0)
{
    MetricsRegistry = std::make_shared<metrics::CleanupRegistry>(client->GetConfig()->MetricRegistry);
}

void AsyncProducer::AsyncClose()
{
    co_start << shutdown();
}

coev::awaitable<int> AsyncProducer::Close()
{
    AsyncClose();

    if (conf_->Producer.Return.Successes)
    {
        co_start << [this]() -> coev::awaitable<void>
        {
            std::shared_ptr<ProducerMessage> msg;
            while ((msg = co_await successes_.get()) != nullptr)
            {
            }
        }();
    }

    std::vector<std::shared_ptr<ProducerError>> pErrs;
    if (conf_->Producer.Return.Errors)
    {
        std::shared_ptr<ProducerError> event;
        while ((event = co_await errors_.get()) != nullptr)
        {
            pErrs.push_back(event);
        }
    }
    else
    {
        auto dummy = co_await errors_.get();
    }

    co_return !pErrs.empty();
}

bool AsyncProducer::IsTransactional()
{
    return txnmgr_->IsTransactional();
}

int AsyncProducer::AddMessageToTxn(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata, const std::string &group_id)
{
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> offsets;
    auto pom = std::make_shared<PartitionOffsetMetadata>();
    pom->Partition = msg->Partition;
    pom->Offset = msg->Offset + 1;
    pom->Metadata = metadata;
    offsets[msg->Topic].push_back(pom);
    return AddOffsetsToTxn(offsets, group_id);
}

int AsyncProducer::AddOffsetsToTxn(const std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> &offsets, const std::string &group_id)
{
    std::lock_guard<std::mutex> lock(tx_lock_);

    if (!IsTransactional())
    {
        return -1;
    }

    return txnmgr_->AddOffsetsToTxn(offsets, group_id);
}

ProducerTxnStatusFlag AsyncProducer::TxnStatus()
{
    return txnmgr_->CurrentTxnStatus();
}

int AsyncProducer::BeginTxn()
{
    std::lock_guard<std::mutex> lock(tx_lock_);

    if (!IsTransactional())
    {
        return -1;
    }

    return txnmgr_->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInTransaction), ErrNoError);
}

coev::awaitable<int> AsyncProducer::CommitTxn()
{
    std::lock_guard<std::mutex> lock(tx_lock_);

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
    std::lock_guard<std::mutex> lock(tx_lock_);

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
    in_flight_++;
    auto msg = std::make_shared<ProducerMessage>();
    if (commit)
    {
        msg->Flags = static_cast<FlagSet>(FlagSet::Endtxn | FlagSet::Committxn);
    }
    else
    {
        msg->Flags = static_cast<FlagSet>(FlagSet::Endtxn | FlagSet::Aborttxn);
    }
    input_.set(msg);

    while (in_flight_ > 0)
    {
        co_await sleep_for(std::chrono::milliseconds(1));
    }

    co_return co_await txnmgr_->FinishTransaction(commit);
}

coev::awaitable<void> AsyncProducer::dispatcher()
{
    std::map<std::string, coev::co_channel<std::shared_ptr<ProducerMessage>>> handlers;
    bool shutting_down = false;

    while (true)
    {
        std::shared_ptr<ProducerMessage> msg = co_await input_.get();
        if (!msg)
        {
            continue;
        }

        if (msg->Flags & FlagSet::Endtxn)
        {
            int err = 0;
            if (msg->Flags & FlagSet::Committxn)
            {
                err = txnmgr_->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagEndTransaction | ProducerTxnFlagCommittingTransaction), err);
            }
            else
            {
                err = txnmgr_->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagEndTransaction | ProducerTxnFlagAbortingTransaction), err);
            }
            if (err != 0)
            {
            }
            in_flight_--;
            continue;
        }

        if (msg->Flags & FlagSet::Shutdown)
        {
            shutting_down = true;
            in_flight_--;
            continue;
        }

        if (msg->Retries == 0)
        {
            if (shutting_down)
            {
                auto pErr = std::make_shared<ProducerError>();
                pErr->Msg = msg;
                pErr->Err = ErrShuttingDown;
                if (conf_->Producer.Return.Errors)
                {
                    errors_.set(pErr);
                }
                continue;
            }
            in_flight_++;

            if (IsTransactional() && (txnmgr_->CurrentTxnStatus() & ProducerTxnFlagInTransaction) == 0)
            {
                returnError(msg, ErrTransactionNotReady);
                continue;
            }
        }

        for (auto &interceptor : conf_->Producer.Interceptors)
        {
            co_await safelyApplyInterceptor(msg, interceptor);
        }

        int version = 1;
        if (conf_->Version.IsAtLeast(V0_11_0_0))
        {
            version = 2;
        }
        else if (!msg->Headers.empty())
        {
            returnError(msg, ErrHeaderVersionIsOld);
            continue;
        }

        int size = msg->ByteSize(version);
        if (size > conf_->Producer.MaxMessageBytes)
        {
            returnError(msg, ErrMaxMessageBytes);
            continue;
        }

        if (auto it = handlers.find(msg->Topic); it != handlers.end())
        {
            it->second.set(msg);
        }
        else
        {
            auto tp = std::make_shared<TopicProducer>(shared_from_this(), msg->Topic);
            task_ << tp->dispatch();
            handlers[msg->Topic].set(msg);
        }
    }
}

coev::awaitable<void> AsyncProducer::retryHandler()
{
    int maxBufferLength = conf_->Producer.Retry.MaxBufferLength;
    if (0 < maxBufferLength && maxBufferLength < minFunctionalRetryBufferLength)
    {
        maxBufferLength = minFunctionalRetryBufferLength;
    }

    int maxBufferBytes = conf_->Producer.Retry.MaxBufferBytes;
    if (0 < maxBufferBytes && maxBufferBytes < minFunctionalRetryBufferBytes)
    {
        maxBufferBytes = minFunctionalRetryBufferBytes;
    }

    int version = 1;
    if (conf_->Version.IsAtLeast(V0_11_0_0))
    {
        version = 2;
    }

    int64_t currentByteSize = 0;
    std::queue<std::shared_ptr<ProducerMessage>> buf;
    while (true)
    {
        auto msg = co_await retries_.get();
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
            if (msgToHandle->Flags == 0)
            {
                currentByteSize -= msgToHandle->ByteSize(version);
            }
        }
        else
        {
            auto msgToHandle = buf.front();
            buf.pop();
            input_.set(msgToHandle);
            currentByteSize -= msgToHandle->ByteSize(version);
        }
    }
}

coev::awaitable<void> AsyncProducer::shutdown()
{
    in_flight_++;
    auto shutdown_msg = std::make_shared<ProducerMessage>();
    shutdown_msg->Flags = Shutdown;
    input_.set(shutdown_msg);

    while (in_flight_ > 0)
    {
        co_await sleep_for(std::chrono::milliseconds(1));
    }

    int err = client_->Close();
    if (err != 0)
    {
        Logger::Println("Error closing client: %s", KErrorToString(err));
    }

    MetricsRegistry->UnregisterAll();
}

coev::awaitable<void> AsyncProducer::bumpIdempotentProducerEpoch()
{

    auto [pid, epoch] = txnmgr_->GetProducerID();
    if (epoch == std::numeric_limits<int16_t>::max())
    {
        std::shared_ptr<TransactionManager> new_txnmgr;
        auto err = co_await NewTransactionManager(conf_, client_, new_txnmgr);
        if (err == (KError)ErrNoError)
        {
            txnmgr_ = new_txnmgr;
        }
    }
    else
    {
        txnmgr_->BumpEpoch();
    }
}

int AsyncProducer::maybeTransitionToErrorState(KError err)
{
    if (err == ErrClusterAuthorizationFailed || err == ErrProducerFenced ||
        err == ErrUnsupportedVersion || err == ErrTransactionalIDAuthorizationFailed)
    {
        return txnmgr_->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagFatalError), err);
    }
    if (txnmgr_->coordinatorSupportsBumpingEpoch_ && (txnmgr_->CurrentTxnStatus() & ProducerTxnFlagEndTransaction) == 0)
    {
        txnmgr_->epochBumpRequired_ = true;
    }
    return txnmgr_->TransitionTo(static_cast<ProducerTxnStatusFlag>(ProducerTxnFlagInError | ProducerTxnFlagAbortableError), err);
}

void AsyncProducer::returnError(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (IsTransactional())
    {
        maybeTransitionToErrorState(err);
    }

    if (!IsTransactional() && msg->hasSequence)
    {
        bumpIdempotentProducerEpoch();
    }

    msg->Clear();
    auto pErr = std::make_shared<ProducerError>();
    pErr->Msg = msg;
    pErr->Err = err;

    if (conf_->Producer.Return.Errors)
    {
        errors_.set(pErr);
    }
    in_flight_--;
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
        if (conf_->Producer.Return.Successes)
        {
            msg->Clear();
            successes_.set(msg);
        }
        in_flight_--;
    }
}

void AsyncProducer::retryMessage(std::shared_ptr<ProducerMessage> msg, KError err)
{
    if (msg->Retries >= conf_->Producer.Retry.Max)
    {
        returnError(msg, err);
    }
    else
    {
        msg->Retries++;
        retries_.set(msg);
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
    for (auto &msg : pSet->msgs)
    {
        if (msg->Retries >= conf_->Producer.Retry.Max)
        {
            returnErrors(pSet->msgs, kerr);
            co_return;
        }
        msg->Retries++;
    }

    std::shared_ptr<Broker> leader;
    int err = co_await client_->Leader(topic, partition, leader);
    if (err != 0)
    {
        for (auto &msg : pSet->msgs)
        {
            returnError(msg, kerr);
        }
        co_return;
    }

    auto bp = getBrokerProducer(leader);
    auto produce_set = std::make_shared<ProduceSet>(shared_from_this());
    produce_set->msgs[topic][partition] = pSet;
    produce_set->bufferBytes += pSet->bufferBytes;
    produce_set->bufferCount += pSet->msgs.size();

    bp->output_.set(produce_set);
    unrefBrokerProducer(leader, bp);
}

std::shared_ptr<BrokerProducer> AsyncProducer::getBrokerProducer(std::shared_ptr<Broker> &broker)
{
    std::lock_guard<std::mutex> lock(broker_lock_);
    auto bp = brokers_[broker];
    if (!bp)
    {
        bp = std::make_shared<BrokerProducer>(shared_from_this(), broker);
        brokers_[broker] = bp;
        broker_refs_[bp] = 0;
    }

    broker_refs_[bp]++;
    return bp;
}

void AsyncProducer::unrefBrokerProducer(std::shared_ptr<Broker> broker, std::shared_ptr<BrokerProducer> bp)
{
    std::lock_guard<std::mutex> lock(broker_lock_);
    broker_refs_[bp]--;
    if (broker_refs_[bp] == 0)
    {
        broker_refs_.erase(bp);

        if (brokers_[broker] == bp)
        {
            brokers_.erase(broker);
        }
    }
}

coev::awaitable<void> AsyncProducer::abandonBrokerConnection(std::shared_ptr<Broker> broker)
{
    std::lock_guard<std::mutex> lock(broker_lock_);

    auto &bc = brokers_[broker];
    auto ok = co_await bc->abandoned_.get();
    if (ok)
    {
        co_await broker->Close();
        brokers_.erase(broker);
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
    producer->task_ << producer->dispatcher();
    producer->task_ << producer->retryHandler();
    co_return ErrNoError;
}

coev::awaitable<int> NewAsyncProducerFromClient(std::shared_ptr<Client> client, std::shared_ptr<AsyncProducer> &producer)
{
    return NewAsyncProducer(client, producer);
}
