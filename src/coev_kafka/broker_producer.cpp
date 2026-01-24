#include "broker_producer.h"

BrokerProducer::BrokerProducer(std::shared_ptr<AsyncProducer> parent, std::shared_ptr<Broker> broker)
    : m_parent(parent), m_broker(broker), m_timer(m_parent->m_conf->Producer.Flush.Frequency.count() / 1000, 0), m_timer_fired(false)
{

    co_start << Run();
}

coev::awaitable<void> BrokerProducer::Run()
{
    coev::co_channel<std::shared_ptr<ProduceSet>> output;
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        if (!(msg = co_await m_input.get()))
        {
            Shutdown();
            co_return;
        }

        if (msg->m_flags & FlagSet::Syn)
        {
            m_current_retries[msg->m_topic][msg->m_partition] = ErrNoError;
            m_parent->m_in_flight--;
            co_return;
        }

        auto reason = NeedsRetry(msg);
        if (reason)
        {
            m_parent->retry_message(msg, reason);

            if (!m_closing && (msg->m_flags & FlagSet::Fin))
            {
                m_current_retries[msg->m_topic].erase(msg->m_partition);
            }
            continue;
        }

        if (msg->m_flags & FlagSet::Fin)
        {
            m_parent->retry_message(msg, ErrShuttingDown);
            continue;
        }

        if (m_buffer->would_overflow(msg))
        {
            auto err = WaitForSpace(msg, false);
            if (err != 0)
            {
                m_parent->retry_message(msg, (KError)err);
                continue;
            }
        }

        auto [producerID, producerEpoch] = m_parent->m_txnmgr->GetProducerID();
        if (producerID != noProducerID && m_buffer->m_producer_epoch != msg->m_producer_epoch)
        {
            int err = WaitForSpace(msg, true);
            if (err != 0)
            {
                m_parent->retry_message(msg, (KError)err);
                continue;
            }
        }

        int err = m_buffer->add(msg);
        if (err != 0)
        {
            m_parent->return_error(msg, err);
            continue;
        }

        if (m_parent->m_conf->Producer.Flush.Frequency > std::chrono::milliseconds(0) && !m_timer.is_active())
        {
            m_timer.reset(m_parent->m_conf->Producer.Flush.Frequency.count() / 1000, 0);
        }

        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            HandleResponse(response);
        }

        if (m_timer_fired || m_buffer->ready_to_flush())
        {
            m_output.set(m_buffer);
        }
    }
}

coev::awaitable<void> BrokerProducer::Shutdown()
{
    while (!m_buffer->empty())
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            HandleResponse(response);
        }
        m_output.set(m_buffer);
        RollOver();
    }

    std::shared_ptr<BrokerProducerResponse> response;
    while ((response = co_await m_responses.get()))
    {
        HandleResponse(response);
    }
}

KError BrokerProducer::NeedsRetry(std::shared_ptr<ProducerMessage> msg)
{
    if (m_closing != ErrNoError)
    {
        return m_closing;
    }

    auto it = m_current_retries.find(msg->m_topic);
    if (it != m_current_retries.end())
    {
        auto pit = it->second.find(msg->m_partition);
        if (pit != it->second.end())
        {
            return pit->second;
        }
    }
    return ErrNoError;
}

int BrokerProducer::WaitForSpace(std::shared_ptr<ProducerMessage> msg, bool force_rollover)
{
    while (true)
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            HandleResponse(response);

            auto reason = NeedsRetry(msg);
            if (reason)
            {
                return -1;
            }
            else if (!m_buffer->would_overflow(msg) && !force_rollover)
            {
                return 0;
            }
        }
        m_output.set(m_buffer);
        RollOver();
        return 0;
    }
}

void BrokerProducer::RollOver()
{
    m_timer.stop();
    m_timer_fired = false;
    m_buffer = std::make_shared<ProduceSet>(m_parent);
}

void BrokerProducer::HandleResponse(std::shared_ptr<BrokerProducerResponse> response)
{
    if (response->m_err)
    {
        HandleError(response->set, response->m_err);
    }
    else
    {
        HandleSuccess(response->set, response->res);
    }

    if (m_buffer->empty())
    {
        RollOver();
    }
}

coev::awaitable<void> BrokerProducer::HandleSuccess(std::shared_ptr<ProduceSet> sent, std::shared_ptr<ProduceResponse> response)
{
    std::vector<std::string> retry_topics;
    sent->each_partition(
        [this, response](const std::string &topic, int32_t partition, auto pSet) -> coev::awaitable<void>
        {
            if (!response)
            {
                m_parent->return_successes(pSet->m_msgs);
                co_return;
            }

            auto block = response->GetBlock(topic, partition);
            if (!block)
            {
                m_parent->return_errors(pSet->m_msgs, ErrIncompleteResponse);
                co_return;
            }

            switch (block->m_err)
            {
            case ErrNoError:
                if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0) && block->m_timestamp != std::chrono::system_clock::time_point{})
                {
                    for (auto &msg : pSet->m_msgs)
                    {
                        msg->m_timestamp = block->m_timestamp;
                    }
                }
                for (size_t i = 0; i < pSet->m_msgs.size(); ++i)
                {
                    pSet->m_msgs[i]->m_offset = block->m_offset + i;
                }
                m_parent->return_successes(pSet->m_msgs);
                break;
            case ErrDuplicateSequenceNumber:
                m_parent->return_successes(pSet->m_msgs);
                break;
            default:
                if (m_parent->m_conf->Producer.Retry.Max <= 0)
                {
                    m_parent->abandon_broker_connection(m_broker);
                    m_parent->return_errors(pSet->m_msgs, block->m_err);
                }
                else
                {
                    retry_topics.push_back(topic);
                }
                break;
            }
        });

    if (!retry_topics.empty())
    {
        if (m_parent->m_conf->Producer.Idempotent)
        {
            int err = co_await m_parent->m_client->RefreshMetadata(retry_topics);
            if (err != 0)
            {
            }
        }

        sent->each_partition(
            [this, response](const std::string &topic, int32_t partition, auto pSet)
            {
                auto block = response->GetBlock(topic, partition);
                if (!block)
                {
                    return;
                }

                switch (block->m_err)
                {
                case ErrInvalidMessage:
                case ErrUnknownTopicOrPartition:
                case ErrLeaderNotAvailable:
                case ErrNotLeaderForPartition:
                case ErrRequestTimedOut:
                case ErrNotEnoughReplicas:
                case ErrNotEnoughReplicasAfterAppend:
                case ErrKafkaStorageError:
                {
                    m_current_retries[topic] = std::map<int32_t, KError>();
                    m_current_retries[topic][partition] = block->m_err;
                    if (m_parent->m_conf->Producer.Idempotent)
                    {
                        co_start << m_parent->retry_batch(topic, partition, pSet, block->m_err);
                    }
                    else
                    {
                        m_parent->retry_messages(pSet->m_msgs, block->m_err);
                    }
                    auto dropped = m_buffer->drop_partition(topic, partition);
                    m_parent->retry_messages(dropped, block->m_err);
                }
                break;
                }
            });
    }
}

void BrokerProducer::HandleError(std::shared_ptr<ProduceSet> _sent, KError err)
{
    auto target = KErrorToString(err);
    if (err != ErrNoError)
    {
        _sent->each_partition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                              { m_parent->return_errors(pSet->m_msgs, err); });
    }
    else
    {
        m_parent->abandon_broker_connection(m_broker);
        m_broker->Close();
        m_closing = err;

        _sent->each_partition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                              { m_parent->retry_messages(pSet->m_msgs, err); });

        m_buffer->each_partition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                                 { m_parent->retry_messages(pSet->m_msgs, err); });

        RollOver();
    }
}
