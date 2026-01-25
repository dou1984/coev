#include "broker_producer.h"

BrokerProducer::BrokerProducer(std::shared_ptr<AsyncProducer> parent, std::shared_ptr<Broker> broker)
    : m_parent(parent), m_broker(broker), m_timer(m_parent->m_conf->Producer.Flush.Frequency.count() / 1000, 0), m_timer_fired(false)
{
    m_buffer = std::make_shared<ProduceSet>(parent);
    m_task << run();
    m_task << bridge();
}

coev::awaitable<void> BrokerProducer::run()
{
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        co_await m_input.get(msg);
        if (msg == nullptr)
        {
            co_await shutdown();
            co_return;
        }

        if (msg->m_flags & FlagSet::Syn)
        {
            m_current_retries[msg->m_topic][msg->m_partition] = ErrNoError;
            m_parent->m_in_flight.done();
            continue;
        }

        auto reason = needs_retry(msg);
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
            auto err = wait_for_space(msg, false);
            if (err != 0)
            {
                m_parent->retry_message(msg, (KError)err);
                continue;
            }
        }

        auto [producerID, producerEpoch] = m_parent->m_txnmgr->GetProducerID();
        if (producerID != noProducerID && m_buffer->m_producer_epoch != msg->m_producer_epoch)
        {
            int err = wait_for_space(msg, true);
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
            handle_response(response);
        }

        if (m_timer_fired || m_buffer->ready_to_flush())
        {
            m_output.set(m_buffer);
        }
    }
}
coev::awaitable<void> BrokerProducer::bridge()
{
    while (true)
    {
        std::shared_ptr<ProduceSet> set;
        co_await m_output.get(set);
        if (!set)
        {
            co_return;
        }

        auto request = set->build_request();
        if (!request)
        {
            auto response = std::make_shared<BrokerProducerResponse>();
            response->set = set;
            response->m_err = ErrInvalidRequest;
            m_responses.set(response);
            continue;
        }

        int16_t version = request->version();
        co_await m_broker->AsyncProduce(*request,
                                        [this, set](ResponsePromise<ProduceResponse> &response)
                                        {
                                            auto broker_response = std::make_shared<BrokerProducerResponse>();
                                            broker_response->set = set;
                                            broker_response->m_err = ErrNoError;
                                            broker_response->res = std::make_shared<ProduceResponse>(response.m_response);
                                            m_responses.set(broker_response);
                                        });
    }
}
coev::awaitable<void> BrokerProducer::shutdown()
{
    while (!m_buffer->empty())
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            handle_response(response);
        }
        m_output.set(m_buffer);
        rollover();
    }

    std::shared_ptr<BrokerProducerResponse> response;
    co_await m_responses.get(response);
    handle_response(response);
}

KError BrokerProducer::needs_retry(std::shared_ptr<ProducerMessage> msg)
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

int BrokerProducer::wait_for_space(std::shared_ptr<ProducerMessage> msg, bool force_rollover)
{
    while (true)
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            handle_response(response);

            auto reason = needs_retry(msg);
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
        rollover();
        return 0;
    }
}

void BrokerProducer::rollover()
{
    m_timer.stop();
    m_timer_fired = false;
    m_buffer = std::make_shared<ProduceSet>(m_parent);
}

void BrokerProducer::handle_response(std::shared_ptr<BrokerProducerResponse> response)
{
    if (response->m_err)
    {
        handle_error(response->set, response->m_err);
    }
    else
    {
        handle_success(response->set, response->res);
    }

    if (m_buffer->empty())
    {
        rollover();
    }
}

coev::awaitable<void> BrokerProducer::handle_success(std::shared_ptr<ProduceSet> sent, std::shared_ptr<ProduceResponse> response)
{
    std::vector<std::string> retry_topics;
    sent->each_partition(
        [this, response, &retry_topics](const std::string &topic, int32_t partition, auto pSet) -> coev::awaitable<void>
        {
            if (!response)
            {
                m_parent->return_successes(pSet->m_messages);
                co_return;
            }

            auto block = response->GetBlock(topic, partition);
            if (!block)
            {
                m_parent->return_errors(pSet->m_messages, ErrIncompleteResponse);
                co_return;
            }

            switch (block->m_err)
            {
            case ErrNoError:
                if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0) && block->m_timestamp != std::chrono::system_clock::time_point{})
                {
                    for (auto &msg : pSet->m_messages)
                    {
                        msg->m_timestamp = block->m_timestamp;
                    }
                }
                for (size_t i = 0; i < pSet->m_messages.size(); ++i)
                {
                    pSet->m_messages[i]->m_offset = block->m_offset + i;
                }
                m_parent->return_successes(pSet->m_messages);
                break;
            case ErrDuplicateSequenceNumber:
                m_parent->return_successes(pSet->m_messages);
                break;
            default:
                if (m_parent->m_conf->Producer.Retry.Max <= 0)
                {
                    m_parent->abandon_broker_connection(m_broker);
                    m_parent->return_errors(pSet->m_messages, block->m_err);
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
                        m_parent->retry_messages(pSet->m_messages, block->m_err);
                    }
                    auto dropped = m_buffer->drop_partition(topic, partition);
                    m_parent->retry_messages(dropped, block->m_err);
                }
                break;
                }
            });
    }
}

void BrokerProducer::handle_error(std::shared_ptr<ProduceSet> _sent, KError err)
{
    auto target = KErrorToString(err);
    if (err != ErrNoError)
    {
        _sent->each_partition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                              { m_parent->return_errors(pSet->m_messages, err); });
    }
    else
    {
        m_parent->abandon_broker_connection(m_broker);
        m_broker->Close();
        m_closing = err;

        _sent->each_partition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                              { m_parent->retry_messages(pSet->m_messages, err); });

        m_buffer->each_partition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                                 { m_parent->retry_messages(pSet->m_messages, err); });

        rollover();
    }
}
