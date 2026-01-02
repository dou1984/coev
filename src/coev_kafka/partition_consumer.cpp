#include "sleep_for.h"
#include "consumer.h"
#include "partition_consumer.h"
#include "broker_consumer.h"
#include "interceptors.h"
void PartitionConsumer::SendError(KError err)
{
    auto cErr = std::make_shared<ConsumerError>();
    cErr->Topic = topic;
    cErr->Partition = partition;
    cErr->Err = err;

    if (conf->Consumer.Return.Errors)
    {
        errors.set(cErr);
    }
    else
    {
        Logger::Println(KErrorToString(err));
    }
}
void PartitionConsumer::SendError(int err)
{
    SendError(static_cast<KError>(err));
}
std::chrono::duration<double> PartitionConsumer::ComputeBackoff()
{
    if (conf->Consumer.Retry.BackoffFunc)
    {
        int32_t retries_val = retries.fetch_add(1) + 1;
        return conf->Consumer.Retry.BackoffFunc(static_cast<int>(retries_val));
    }
    return conf->Consumer.Retry.Backoff;
}

coev::awaitable<int> PartitionConsumer::Dispatcher()
{
    while (true)
    {
        bool dummy = co_await trigger.get();

        bool dying_received = false;
        if (dying.try_get(dummy))
        {
            dying_received = true;
        }

        if (dying_received)
        {
            break;
        }

        co_await sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(ComputeBackoff()));

        if (broker != nullptr)
        {
            consumer->unrefBrokerConsumer(broker);
            broker = nullptr;
        }

        auto err = co_await Dispatch();
        if (err)
        {
            SendError(err);
            trigger.set(true);
        }
    }

    if (broker != nullptr)
    {
        consumer->unrefBrokerConsumer(broker);
    }
    consumer->removeChild(shared_from_this());
    co_return 0;
}

coev::awaitable<int> PartitionConsumer::preferredBroker(std::shared_ptr<Broker> &broker, int32_t &epoch)
{

    int err = 0;
    if (preferredReadReplica >= 0)
    {
        err = co_await consumer->client->GetBroker(preferredReadReplica, broker);
        if (err != ErrNoError)
        {
            epoch = leaderEpoch;
            co_return 0;
        }

        preferredReadReplica = invalidPreferredReplicaID;

        std::vector<std::string> topics = {topic};
        err = co_await consumer->client->RefreshMetadata(topics);
        if (err != ErrNoError)
        {
            co_return err;
        }
    }

    err = co_await consumer->client->LeaderAndEpoch(topic, partition, broker, epoch);
    if (err != 0)
    {
        co_return err;
    }
    co_return err;
}

coev::awaitable<int> PartitionConsumer::Dispatch()
{

    std::vector<std::string> topics = {topic};
    int err = co_await consumer->client->RefreshMetadata(topics);
    if (err)
        co_return err;

    std::shared_ptr<Broker> broker;
    err = co_await preferredBroker(broker, leaderEpoch);
    if (err)
        co_return err;

    auto brokerConsumer = consumer->refBrokerConsumer(broker);
    brokerConsumer->input.set(shared_from_this());

    co_return ErrNoError;
}

coev::awaitable<int> PartitionConsumer::ChooseStartingOffset(int64_t offset)
{
    int64_t newestOffset;
    auto err = co_await consumer->client->GetOffset(topic, partition, OffsetNewest, newestOffset);
    if (err)
        co_return err;
    highWaterMarkOffset.store(newestOffset);

    int64_t oldestOffset;
    err = co_await consumer->client->GetOffset(topic, partition, OffsetOldest, oldestOffset);
    if (err)
        co_return err;

    if (offset == OffsetNewest)
    {
        offset = newestOffset;
    }
    else if (offset == OffsetOldest)
    {
        offset = oldestOffset;
    }
    else if (offset >= oldestOffset && offset <= newestOffset)
    {
        offset = offset;
    }
    else
    {
        co_return ErrOffsetOutOfRange;
    }

    co_return ErrNoError;
}

void PartitionConsumer::AsyncClose()
{
    dying.set(true);
}

coev::awaitable<int> PartitionConsumer::Close()
{
    AsyncClose();

    std::vector<std::shared_ptr<ConsumerError>> consumerErrors;
    std::shared_ptr<ConsumerError> err;
    while ((err = co_await errors.get()))
    {
        consumerErrors.push_back(err);
    }

    co_return 0;
}

int64_t PartitionConsumer::HighWaterMarkOffset()
{
    return highWaterMarkOffset.load();
}

coev::awaitable<void> PartitionConsumer::ResponseFeeder()
{
    std::vector<std::shared_ptr<ConsumerMessage>> msgs;
    auto expiryTime = conf->Consumer.MaxProcessingTime;
    auto lastExpiry = std::chrono::steady_clock::now();
    bool firstAttempt = true;

    while (true)
    {
        std::shared_ptr<FetchResponse> response = co_await feeder.get();
        responseResult = (KError)ParseResponse(response, msgs);

        if (responseResult != ErrNoError)
        {
            retries.store(0);
        }

        for (size_t i = 0; i < msgs.size(); ++i)
        {
            auto msg = msgs[i];
            Interceptors(msg);

            bool sent = false;
            bool died = false;
            auto now = std::chrono::steady_clock::now();
            if (now - lastExpiry >= expiryTime)
            {
                if (!firstAttempt)
                {
                    responseResult = ErrTimedOut;
                    broker->acks.done();
                    for (size_t j = i; j < msgs.size(); ++j)
                    {
                        Interceptors(msgs[j]);
                        messages.set(msgs[j]);

                        if (dying.try_get(died) && died)
                            break;
                    }
                    broker->input.set(shared_from_this());
                    break;
                }
                else
                {
                    firstAttempt = false;
                    --i;
                    continue;
                }
            }

            if (dying.try_get(died) && died)
            {
                broker->acks.done();
                break;
            }

            messages.set(msg);
            firstAttempt = true;
        }

        broker->acks.done();
    }
    co_return;
}

int PartitionConsumer::ParseMessages(std::shared_ptr<MessageSet> msgSet, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{

    for (auto &msgBlock : msgSet->Messages)
    {
        auto blockMsgs = msgBlock->Messages();
        for (auto &msg : blockMsgs)
        {
            int64_t offset = msg->Offset;
            auto timestamp = msg->Msg->Timestamp_;
            if (msg->Msg->Version >= 1)
            {
                int64_t baseOffset = msgBlock->Offset - blockMsgs[blockMsgs.size() - 1]->Offset;
                offset += baseOffset;
                if (msg->Msg->LogAppendTime)
                {
                    timestamp = msgBlock->Msg->Timestamp_;
                }
            }
            if (offset < offset)
                continue;
            auto cm = std::make_shared<ConsumerMessage>();
            cm->Topic = topic;
            cm->Partition = partition;
            cm->Key = msg->Msg->Key;
            cm->Value = msg->Msg->Value;
            cm->Offset = offset;
            cm->Timestamp = timestamp.get_time();
            cm->BlockTimestamp = msgBlock->Msg->Timestamp_.get_time();
            messages.push_back(cm);
            offset = offset + 1;
        }
    }
    if (messages.empty())
    {
        offset++;
    }
    return ErrNoError;
}

int PartitionConsumer::ParseRecords(std::shared_ptr<RecordBatch> batch, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{

    messages.reserve(batch->Records.size());

    for (auto &rec : batch->Records)
    {
        int64_t offset = batch->FirstOffset + rec->OffsetDelta;
        if (offset < offset)
            continue;
        auto timestamp = batch->FirstTimestamp + std::chrono::milliseconds(rec->TimestampDelta.count());
        if (batch->LogAppendTime)
        {
            timestamp = batch->MaxTimestamp;
        }
        auto cm = std::make_shared<ConsumerMessage>();
        cm->Topic = topic;
        cm->Partition = partition;
        cm->Key = rec->Key;
        cm->Value = rec->Value;
        cm->Offset = offset;
        cm->Timestamp = timestamp;
        cm->Headers = rec->Headers;
        messages.push_back(cm);
        offset = offset + 1;
    }
    if (messages.empty())
    {
        offset++;
    }
    return 0;
}

int PartitionConsumer::ParseResponse(std::shared_ptr<FetchResponse> response, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{
    std::shared_ptr<metrics::Histogram> consumerBatchSizeMetric;
    if (consumer && consumer->metricRegistry)
    {
        consumerBatchSizeMetric = metrics::GetOrRegisterHistogram("consumer-batch-size", consumer->metricRegistry);
    }

    if (response->ThrottleTime.count() != 0 && response->Blocks.empty())
    {
        Logger::Printf("consumer/broker/%d FetchResponse throttled %v\n",
                       broker->broker->ID(), std::chrono::duration_cast<std::chrono::milliseconds>(response->ThrottleTime).count());
        return ErrThrottled;
    }

    auto block = response->GetBlock(topic, partition);
    if (!block)
    {
        return ErrIncompleteResponse;
    }

    if (!errorsIs(block->Err, ErrNoError))
    {
        return block->Err;
    }

    auto nRecs = block->numRecords();

    if (consumerBatchSizeMetric)
    {
        consumerBatchSizeMetric->Update(static_cast<int64_t>(nRecs));
    }

    if (block->PreferredReadReplica != invalidPreferredReplicaID)
    {
        preferredReadReplica = block->PreferredReadReplica;
    }

    if (nRecs == 0)
    {
        std::shared_ptr<ConsumerMessage> partialTrailingMessage;
        bool isPartial;
        auto err = block->isPartial(isPartial);
        if (!isPartial)
        {
            return err;
        }

        if (partialTrailingMessage)
        {
            if (conf->Consumer.Fetch.Max > 0 && fetchSize == conf->Consumer.Fetch.Max)
            {
                SendError(ErrMessageTooLarge);
                offset++;
            }
            else
            {
                fetchSize *= 2;
                if (fetchSize < 0)
                {
                    fetchSize = INT32_MAX;
                }
                if (conf->Consumer.Fetch.Max > 0 && fetchSize > conf->Consumer.Fetch.Max)
                {
                    fetchSize = conf->Consumer.Fetch.Max;
                }
            }
        }
        else if (block->recordsNextOffset <= block->HighWaterMarkOffset)
        {
            Logger::Printf("consumer/broker/%d received batch with zero records but high watermark was not reached, topic %s, partition %d, next offset %ld\n",
                           broker->broker->ID(), topic.c_str(), partition, block->recordsNextOffset);
            offset = block->recordsNextOffset;
        }
        return 0;
    }

    fetchSize = conf->Consumer.Fetch.DefaultVal;
    highWaterMarkOffset.store(block->HighWaterMarkOffset);

    std::unordered_set<int64_t> abortedProducerIDs;
    auto abortedTransactions = block->getAbortedTransactions();

    for (auto &records : block->RecordsSet)
    {
        if (records->RecordsType == legacyRecords)
        {
            std::vector<std::shared_ptr<ConsumerMessage>> msgSetMsgs;
            auto err = ParseMessages(records->MsgSet, msgSetMsgs);
            if (err)
                return err;
            messages.insert(messages.end(), msgSetMsgs.begin(), msgSetMsgs.end());
        }
        else if (records->RecordsType == defaultRecords)
        {
            for (auto it = abortedTransactions.begin(); it != abortedTransactions.end();)
            {
                if ((*it)->FirstOffset > records->RBatch->LastOffset())
                    break;
                abortedProducerIDs.insert((*it)->ProducerID);
                it = abortedTransactions.erase(it);
            }
            std::vector<std::shared_ptr<ConsumerMessage>> batchMsgs;
            auto err = ParseRecords(records->RBatch, batchMsgs);
            if (err)
                return err;

            bool isControl;
            auto controlErr = records->isControl(isControl);
            if (controlErr)
            {
                if (conf->Consumer.IsolationLevel_ == ReadCommitted)
                {
                    return controlErr;
                }
                continue;
            }

            if (isControl)
            {
                ControlRecord control;
                auto err = records->getControlRecord(control);
                if (err)
                    return err;

                if (control.Type == ControlRecordAbort)
                {
                    abortedProducerIDs.erase(records->RBatch->ProducerID);
                }
                continue;
            }

            if (conf->Consumer.IsolationLevel_ == ReadCommitted)
            {
                if (records->RBatch->IsTransactional && abortedProducerIDs.count(records->RBatch->ProducerID))
                {
                    continue;
                }
            }

            messages.insert(messages.end(), batchMsgs.begin(), batchMsgs.end());
        }
        else
        {
            return ErrUnknownRecordsType;
        }
    }

    return 0;
}

coev::awaitable<int> PartitionConsumer::Interceptors(std::shared_ptr<ConsumerMessage> &msg)
{
    for (auto &interceptor : conf->Consumer.Interceptors)
    {
        auto err = co_await safelyApplyInterceptor(msg, interceptor);
        if (err)
            co_return err;
    }
    co_return ErrNoError;
}

void PartitionConsumer::Pause()
{
    paused.store(true);
}

void PartitionConsumer::Resume()
{
    paused.store(false);
}
void PartitionConsumer::PauseAll()
{
}
void PartitionConsumer::ResumeAll()
{
}
bool PartitionConsumer::IsPaused()
{
    return paused.load();
}
