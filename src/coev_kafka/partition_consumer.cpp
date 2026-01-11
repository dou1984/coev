#include "sleep_for.h"
#include "consumer.h"
#include "partition_consumer.h"
#include "broker_consumer.h"
#include "interceptors.h"
void PartitionConsumer::SendError(KError err)
{
    auto cErr = std::make_shared<ConsumerError>();
    cErr->m_topic = m_topic;
    cErr->m_partition = m_partition;
    cErr->m_err = err;

    if (m_conf->Consumer.Return.Errors)
    {
        m_errors.set(cErr);
    }
    else
    {
        LOG_CORE("%s\n", KErrorToString(err));
    }
}
void PartitionConsumer::SendError(int err)
{
    SendError(static_cast<KError>(err));
}
std::chrono::duration<double> PartitionConsumer::ComputeBackoff()
{
    if (m_conf->Consumer.Retry.BackoffFunc)
    {
        int32_t retries_val = m_retries.fetch_add(1) + 1;
        return m_conf->Consumer.Retry.BackoffFunc(static_cast<int>(retries_val));
    }
    return m_conf->Consumer.Retry.Backoff;
}

coev::awaitable<int> PartitionConsumer::Dispatcher()
{
    while (true)
    {
        bool dummy = co_await m_trigger.get();

        bool dying_received = false;
        if (m_dying.try_get(dummy))
        {
            dying_received = true;
        }

        if (dying_received)
        {
            break;
        }

        co_await sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(ComputeBackoff()));

        if (m_broker != nullptr)
        {
            m_consumer->unrefBrokerConsumer(m_broker);
            m_broker = nullptr;
        }

        auto err = co_await Dispatch();
        if (err)
        {
            SendError(err);
            m_trigger.set(true);
        }
    }

    if (m_broker != nullptr)
    {
        m_consumer->unrefBrokerConsumer(m_broker);
    }
    m_consumer->removeChild(shared_from_this());
    co_return 0;
}

coev::awaitable<int> PartitionConsumer::preferredBroker(std::shared_ptr<Broker> &broker, int32_t &epoch)
{

    int err = 0;
    if (m_preferred_read_replica >= 0)
    {
        err = co_await m_consumer->m_client->GetBroker(m_preferred_read_replica, broker);
        if (err != ErrNoError)
        {
            epoch = m_leader_epoch;
            co_return 0;
        }

        m_preferred_read_replica = invalidPreferredReplicaID;

        std::vector<std::string> topics = {m_topic};
        err = co_await m_consumer->m_client->RefreshMetadata(topics);
        if (err != ErrNoError)
        {
            co_return err;
        }
    }

    err = co_await m_consumer->m_client->LeaderAndEpoch(m_topic, m_partition, broker, epoch);
    if (err != 0)
    {
        co_return err;
    }
    co_return err;
}

coev::awaitable<int> PartitionConsumer::Dispatch()
{

    std::vector<std::string> topics = {m_topic};
    int err = co_await m_consumer->m_client->RefreshMetadata(topics);
    if (err)
        co_return err;

    std::shared_ptr<Broker> broker;
    err = co_await preferredBroker(broker, m_leader_epoch);
    if (err)
        co_return err;

    auto brokerConsumer = m_consumer->refBrokerConsumer(broker);
    brokerConsumer->m_input.set(shared_from_this());

    co_return ErrNoError;
}

coev::awaitable<int> PartitionConsumer::ChooseStartingOffset(int64_t offset)
{
    int64_t newestOffset;
    auto err = co_await m_consumer->m_client->GetOffset(m_topic, m_partition, OffsetNewest, newestOffset);
    if (err)
        co_return err;
    m_high_water_mark_offset.store(newestOffset);

    int64_t oldestOffset;
    err = co_await m_consumer->m_client->GetOffset(m_topic, m_partition, OffsetOldest, oldestOffset);
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
    m_dying.set(true);
}

coev::awaitable<int> PartitionConsumer::Close()
{
    AsyncClose();

    std::vector<std::shared_ptr<ConsumerError>> consumerErrors;
    std::shared_ptr<ConsumerError> err;
    while ((err = co_await m_errors.get()))
    {
        consumerErrors.push_back(err);
    }

    co_return 0;
}

int64_t PartitionConsumer::HighWaterMarkOffset()
{
    return m_high_water_mark_offset.load();
}

coev::awaitable<void> PartitionConsumer::ResponseFeeder()
{
    std::vector<std::shared_ptr<ConsumerMessage>> msgs;
    auto expiryTime = m_conf->Consumer.MaxProcessingTime;
    auto lastExpiry = std::chrono::steady_clock::now();
    bool firstAttempt = true;

    while (true)
    {
        std::shared_ptr<FetchResponse> response = co_await m_feeder.get();
        m_response_result = (KError)ParseResponse(response, msgs);

        if (m_response_result != ErrNoError)
        {
            m_retries.store(0);
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
                    m_response_result = ErrTimedOut;
                    m_broker->m_acks.done();
                    for (size_t j = i; j < msgs.size(); ++j)
                    {
                        Interceptors(msgs[j]);
                        m_messages.set(msgs[j]);

                        if (m_dying.try_get(died) && died)
                            break;
                    }
                    m_broker->m_input.set(shared_from_this());
                    break;
                }
                else
                {
                    firstAttempt = false;
                    --i;
                    continue;
                }
            }

            if (m_dying.try_get(died) && died)
            {
                m_broker->m_acks.done();
                break;
            }

            m_messages.set(msg);
            firstAttempt = true;
        }

        m_broker->m_acks.done();
    }
    co_return;
}

int PartitionConsumer::ParseMessages(std::shared_ptr<MessageSet> msgSet, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{

    for (auto &msgBlock : msgSet->m_messages)
    {
        auto blockMsgs = msgBlock->Messages();
        for (auto &msg : blockMsgs)
        {
            int64_t offset = msg->m_offset;
            auto timestamp = msg->m_msg->m_timestamp;
            if (msg->m_msg->m_version >= 1)
            {
                int64_t baseOffset = msgBlock->m_offset - blockMsgs[blockMsgs.size() - 1]->m_offset;
                offset += baseOffset;
                if (msg->m_msg->m_log_append_time)
                {
                    timestamp = msgBlock->m_msg->m_timestamp;
                }
            }
            if (offset < m_offset)
                continue;
            auto cm = std::make_shared<ConsumerMessage>();
            cm->m_topic = m_topic;
            cm->m_partition = m_partition;
            cm->m_key = msg->m_msg->m_key;
            cm->m_value = msg->m_msg->m_value;
            cm->m_offset = offset;
            cm->m_timestamp = timestamp.get_time();
            cm->m_block_timestamp = msgBlock->m_msg->m_timestamp.get_time();
            messages.push_back(cm);
            m_offset = offset + 1;
        }
    }
    if (messages.empty())
    {
        m_offset++;
    }
    return ErrNoError;
}

int PartitionConsumer::ParseRecords(std::shared_ptr<RecordBatch> batch, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{

    messages.reserve(batch->m_records.size());

    for (auto &rec : batch->m_records)
    {
        int64_t offset = batch->m_first_offset + rec->m_offset_delta;
        if (offset < m_offset)
            continue;
        auto timestamp = batch->m_first_timestamp + std::chrono::milliseconds(rec->m_timestamp_delta.count());
        if (batch->m_log_append_time)
        {
            timestamp = batch->m_max_timestamp;
        }
        auto cm = std::make_shared<ConsumerMessage>();
        cm->m_topic = m_topic;
        cm->m_partition = m_partition;
        cm->m_key = rec->m_key;
        cm->m_value = rec->m_value;
        cm->m_offset = offset;
        cm->m_timestamp = timestamp;
        cm->m_headers = rec->m_headers;
        messages.push_back(cm);
        m_offset = offset + 1;
    }
    if (messages.empty())
    {
        m_offset++;
    }
    return 0;
}

int PartitionConsumer::ParseResponse(std::shared_ptr<FetchResponse> response, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{
    std::shared_ptr<metrics::Histogram> consumerBatchSizeMetric;
    if (m_consumer && m_consumer->m_metric_registry)
    {
        consumerBatchSizeMetric = metrics::GetOrRegisterHistogram("consumer-batch-size", m_consumer->m_metric_registry);
    }

    if (response->m_throttle_time.count() != 0 && response->m_blocks.empty())
    {
        LOG_CORE("FetchResponse throttled %ldms from broker %d\n",
                 std::chrono::duration_cast<std::chrono::milliseconds>(response->m_throttle_time).count(), m_broker->m_broker->ID());
        return ErrThrottled;
    }

    auto block = response->GetBlock(m_topic, m_partition);
    if (!block)
    {
        return ErrIncompleteResponse;
    }

    if (!errorsIs(block->m_err, ErrNoError))
    {
        return block->m_err;
    }

    auto nRecs = block->numRecords();

    if (consumerBatchSizeMetric)
    {
        consumerBatchSizeMetric->Update(static_cast<int64_t>(nRecs));
    }

    if (block->m_preferred_read_replica != invalidPreferredReplicaID)
    {
        m_preferred_read_replica = block->m_preferred_read_replica;
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
            if (m_conf->Consumer.Fetch.Max > 0 && m_fetch_size == m_conf->Consumer.Fetch.Max)
            {
                SendError(ErrMessageTooLarge);
                m_offset++;
            }
            else
            {
                m_fetch_size *= 2;
                if (m_fetch_size < 0)
                {
                    m_fetch_size = INT32_MAX;
                }
                if (m_conf->Consumer.Fetch.Max > 0 && m_fetch_size > m_conf->Consumer.Fetch.Max)
                {
                    m_fetch_size = m_conf->Consumer.Fetch.Max;
                }
            }
        }
        else if (block->m_records_next_offset <= block->m_high_water_mark_offset)
        {
            LOG_DBG("received batch with zero records but high watermark was not reached, topic %s, partition %d, next offset %ld, broker %d\n",
                    m_topic.c_str(), m_partition, block->m_records_next_offset, m_broker->m_broker->ID());
            m_offset = block->m_records_next_offset;
        }
        return 0;
    }

    m_fetch_size = m_conf->Consumer.Fetch.DefaultVal;
    m_high_water_mark_offset.store(block->m_high_water_mark_offset);

    std::unordered_set<int64_t> abortedProducerIDs;
    auto abortedTransactions = block->getAbortedTransactions();

    for (auto &records : block->m_records_set)
    {
        if (records->m_records_type == legacyRecords)
        {
            std::vector<std::shared_ptr<ConsumerMessage>> msgSetMsgs;
            auto err = ParseMessages(records->m_msg_set, msgSetMsgs);
            if (err)
                return err;
            messages.insert(messages.end(), msgSetMsgs.begin(), msgSetMsgs.end());
        }
        else if (records->m_records_type == defaultRecords)
        {
            for (auto it = abortedTransactions.begin(); it != abortedTransactions.end();)
            {
                if ((*it)->m_first_offset > records->m_record_batch->LastOffset())
                    break;
                abortedProducerIDs.insert((*it)->m_producer_id);
                it = abortedTransactions.erase(it);
            }
            std::vector<std::shared_ptr<ConsumerMessage>> batchMsgs;
            auto err = ParseRecords(records->m_record_batch, batchMsgs);
            if (err)
                return err;

            bool isControl;
            auto controlErr = records->isControl(isControl);
            if (controlErr)
            {
                if (m_conf->Consumer.IsolationLevel_ == ReadCommitted)
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

                if (control.m_type == ControlRecordType::ControlRecordAbort)
                {
                    abortedProducerIDs.erase(records->m_record_batch->m_producer_id);
                }
                continue;
            }

            if (m_conf->Consumer.IsolationLevel_ == ReadCommitted)
            {
                if (records->m_record_batch->m_is_transactional && abortedProducerIDs.count(records->m_record_batch->m_producer_id))
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
    for (auto &interceptor : m_conf->Consumer.Interceptors)
    {
        auto err = co_await safelyApplyInterceptor(msg, interceptor);
        if (err)
            co_return err;
    }
    co_return ErrNoError;
}

void PartitionConsumer::Pause()
{
    m_paused.store(true);
}

void PartitionConsumer::Resume()
{
    m_paused.store(false);
}
void PartitionConsumer::PauseAll()
{
}
void PartitionConsumer::ResumeAll()
{
}
bool PartitionConsumer::IsPaused()
{
    return m_paused.load();
}
