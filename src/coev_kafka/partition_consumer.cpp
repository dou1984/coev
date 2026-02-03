#include <unordered_set>
#include "sleep_for.h"
#include "consumer.h"
#include "partition_consumer.h"
#include "broker_consumer.h"
#include "interceptors.h"

void PartitionConsumer::SendError(KError err)
{
    auto cerr = std::make_shared<ConsumerError>();
    cerr->m_topic = m_topic;
    cerr->m_partition = m_partition;
    cerr->m_err = err;

    if (m_conf->Consumer.Return.Errors)
    {
        m_errors.set(cerr);
    }
    else
    {
        LOG_CORE("%s", KErrorToString(err));
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
        bool dummy;
        co_await m_trigger.get(dummy);

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
        auto err = co_await Dispatch();
        if (err)
        {
            SendError(err);
            m_trigger.set(true);
        }
    }

    m_consumer->RemoveChild(shared_from_this());
    co_return 0;
}

coev::awaitable<int> PartitionConsumer::PreferredBroker(std::shared_ptr<Broker> &broker, int32_t &epoch)
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

        m_preferred_read_replica = InvalidPreferredReplicaID;

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
    {
        co_return err;
    }

    std::shared_ptr<Broker> broker;
    err = co_await PreferredBroker(broker, m_leader_epoch);
    if (err)
    {
        co_return err;
    }

    auto brokerConsumer = m_consumer->RefBrokerConsumer(broker);
    brokerConsumer->m_input.set(shared_from_this());
    co_return ErrNoError;
}

coev::awaitable<int> PartitionConsumer::ChooseStartingOffset(int64_t offset)
{
    int64_t newestOffset;
    auto err = co_await m_consumer->m_client->GetOffset(m_topic, m_partition, OffsetNewest, newestOffset);
    if (err)
    {
        co_return err;
    }
    m_high_water_mark_offset.store(newestOffset);

    int64_t oldestOffset;
    err = co_await m_consumer->m_client->GetOffset(m_topic, m_partition, OffsetOldest, oldestOffset);
    if (err)
    {
        co_return err;
    }

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
    while (true)
    {
        co_await m_errors.get(err);
        if (!err)
        {
            break;
        }
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
    while (true)
    {
        std::shared_ptr<FetchResponse> response;
        co_await m_feeder.get(response);

        std::vector<std::shared_ptr<ConsumerMessage>> messages;
        auto expiry_time = m_conf->Consumer.MaxProcessingTime;
        auto last_expiry = std::chrono::steady_clock::now();
        bool first_attempt = true;

        m_response_result = (KError)ParseResponse(response, messages);
        if (m_response_result != ErrNoError)
        {
            m_retries.store(0);
        }
        for (size_t i = 0; i < messages.size(); ++i)
        {
            auto msg = messages[i];
            Interceptors(msg);

            bool sent = false;
            bool died = false;
            auto now = std::chrono::steady_clock::now();
            if (now - last_expiry >= expiry_time)
            {
                if (!first_attempt)
                {
                    m_response_result = ErrTimedOut;

                    for (size_t j = i; j < messages.size(); ++j)
                    {
                        Interceptors(messages[j]);
                        m_messages.set(messages[j]);

                        if (m_dying.try_get(died) && died)
                        {
                            continue;
                        }
                    }
                    m_broker->m_input.set(shared_from_this());
                    continue;
                }
                else
                {
                    first_attempt = false;
                    --i;
                    continue;
                }
            }

            if (m_dying.try_get(died) && died)
            {
                co_return;
            }

            m_messages.set(msg);
            first_attempt = true;
        }
    }
}

int PartitionConsumer::ParseMessages(std::shared_ptr<MessageSet> msg_set, std::vector<std::shared_ptr<ConsumerMessage>> &messages)
{

    for (auto &block : msg_set->m_messages)
    {
        auto msgs = block.Messages();
        for (auto &msg : msgs)
        {
            int64_t offset = msg.m_offset;
            auto timestamp = msg.m_message->m_timestamp;
            if (msg.m_message->m_version >= 1)
            {
                int64_t base_offset = block.m_offset - msgs.back().m_offset;
                offset += base_offset;
                if (msg.m_message->m_log_append_time)
                {
                    timestamp = block.m_message->m_timestamp;
                }
            }
            if (offset < m_offset)
            {
                continue;
            }
            auto cm = std::make_shared<ConsumerMessage>();
            cm->m_topic = m_topic;
            cm->m_partition = m_partition;
            cm->m_key = msg.m_message->m_key;
            cm->m_value = msg.m_message->m_value;
            cm->m_offset = offset;
            cm->m_timestamp = timestamp.get_time();
            cm->m_block_timestamp = block.m_message->m_timestamp.get_time();
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

    if (response->m_throttle_time.count() != 0 && response->m_blocks.empty())
    {
        LOG_CORE("FetchResponse throttled %ldms from broker %d",
                 std::chrono::duration_cast<std::chrono::milliseconds>(response->m_throttle_time).count(), m_broker->m_broker->ID());
        return ErrThrottled;
    }

    auto &block = response->get_block(m_topic, m_partition);

    if (!errorsIs(block.m_err, ErrNoError))
    {
        return block.m_err;
    }

    auto nRecs = block.num_records();

    if (block.m_preferred_read_replica != InvalidPreferredReplicaID)
    {
        m_preferred_read_replica = block.m_preferred_read_replica;
    }

    if (nRecs == 0)
    {
        std::shared_ptr<ConsumerMessage> partialTrailingMessage;
        bool isPartial;
        auto err = block.is_partial(isPartial);
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
        else if (block.m_records_next_offset <= block.m_high_water_mark_offset)
        {
            LOG_DBG("received batch with zero records but high watermark was not reached, topic %s, partition %d, next offset %ld, broker %d",
                    m_topic.c_str(), m_partition, block.m_records_next_offset, m_broker->m_broker->ID());
            m_offset = block.m_records_next_offset;
        }
        return 0;
    }

    m_fetch_size = m_conf->Consumer.Fetch.DefaultVal;
    m_high_water_mark_offset.store(block.m_high_water_mark_offset);

    std::unordered_set<int64_t> aborted_producer_ids;
    auto &aborted_transactions = block.get_aborted_transactions();

    for (auto &records : block.m_records_set)
    {
        if (records.m_records_type == LegacyRecords)
        {
            std::vector<std::shared_ptr<ConsumerMessage>> msg_set;
            auto err = ParseMessages(records.m_message_set, msg_set);
            if (err)
            {
                return err;
            }
            messages.insert(messages.end(), msg_set.begin(), msg_set.end());
        }
        else if (records.m_records_type == DefaultRecords)
        {
            for (auto it = aborted_transactions.begin(); it != aborted_transactions.end();)
            {
                if (it->m_first_offset > records.m_record_batch->last_offset())
                {
                    break;
                }
                aborted_producer_ids.insert(it->m_producer_id);
                it = aborted_transactions.erase(it);
            }
            std::vector<std::shared_ptr<ConsumerMessage>> batch_messages;
            auto err = ParseRecords(records.m_record_batch, batch_messages);
            if (err)
            {
                return err;
            }

            bool isControl;
            err = records.is_control(isControl);
            if (err)
            {
                if (m_conf->Consumer.IsolationLevel_ == ReadCommitted)
                {
                    return err;
                }
                continue;
            }

            if (isControl)
            {
                ControlRecord control;
                auto err = records.get_control_record(control);
                if (err)
                {
                    return err;
                }

                if (control.m_type == ControlRecordType::ControlRecordAbort)
                {
                    aborted_producer_ids.erase(records.m_record_batch->m_producer_id);
                }
                continue;
            }

            if (m_conf->Consumer.IsolationLevel_ == ReadCommitted)
            {
                if (records.m_record_batch->m_is_transactional && aborted_producer_ids.count(records.m_record_batch->m_producer_id))
                {
                    continue;
                }
            }

            messages.insert(messages.end(), batch_messages.begin(), batch_messages.end());
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
        auto err = co_await SafelyApplyInterceptor(msg, interceptor);
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
