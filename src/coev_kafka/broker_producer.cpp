#include "broker_producer.h"

BrokerProducer::BrokerProducer(std::shared_ptr<AsyncProducer> parent, std::shared_ptr<Broker> broker)
    : m_parent(parent), m_broker(broker), m_timer(m_parent->m_conf->Producer.Flush.Frequency.count() / 1000, 0), m_timer_fired(false)
{

    co_start << run();
}

coev::awaitable<void> BrokerProducer::run()
{
    coev::co_channel<std::shared_ptr<ProduceSet>> output;
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        if ((msg = co_await m_input.get()))
        {
            shutdown();
            co_return;
        }

        if (msg->m_flags & FlagSet::Syn)
        {
            m_current_retries[msg->m_topic][msg->m_partition] = ErrNoError;
            m_parent->m_in_flight--;
            co_return;
        }

        auto reason = needsRetry(msg);
        if (reason)
        {
            m_parent->retryMessage(msg, reason);

            if (!m_closing && (msg->m_flags & FlagSet::Fin))
            {
                m_current_retries[msg->m_topic].erase(msg->m_partition);
            }
            continue;
        }

        if (msg->m_flags & FlagSet::Fin)
        {
            m_parent->retryMessage(msg, ErrShuttingDown);
            continue;
        }

        if (m_buffer->WouldOverflow(msg))
        {
            auto err = waitForSpace(msg, false);
            if (err != 0)
            {
                m_parent->retryMessage(msg, (KError)err);
                continue;
            }
        }

        auto [producerID, producerEpoch] = m_parent->m_txnmgr->GetProducerID();
        if (producerID != noProducerID && m_buffer->m_producer_epoch != msg->m_producer_epoch)
        {
            int err = waitForSpace(msg, true);
            if (err != 0)
            {
                m_parent->retryMessage(msg, (KError)err);
                continue;
            }
        }

        int err = m_buffer->Add(msg);
        if (err != 0)
        {
            m_parent->returnError(msg, err);
            continue;
        }

        if (m_parent->m_conf->Producer.Flush.Frequency > std::chrono::milliseconds(0) && !m_timer.is_active())
        {
            m_timer.reset(m_parent->m_conf->Producer.Flush.Frequency.count() / 1000, 0);
        }

        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            handleResponse(response);
        }

        if (m_timer_fired || m_buffer->ReadyToFlush())
        {
            m_output.set(m_buffer);
        }
    }
}

coev::awaitable<void> BrokerProducer::shutdown()
{
    while (!m_buffer->Empty())
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            handleResponse(response);
        }
        m_output.set(m_buffer);
        rollOver();
    }

    std::shared_ptr<BrokerProducerResponse> response;
    while ((response = co_await m_responses.get()))
    {
        handleResponse(response);
    }
}

KError BrokerProducer::needsRetry(std::shared_ptr<ProducerMessage> msg)
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

int BrokerProducer::waitForSpace(std::shared_ptr<ProducerMessage> msg, bool force_rollover)
{
    while (true)
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (m_responses.try_get(response))
        {
            handleResponse(response);

            auto reason = needsRetry(msg);
            if (reason)
            {
                return -1;
            }
            else if (!m_buffer->WouldOverflow(msg) && !force_rollover)
            {
                return 0;
            }
        }
        m_output.set(m_buffer);
        rollOver();
        return 0;
    }
}

void BrokerProducer::rollOver()
{
    m_timer.stop();
    m_timer_fired = false;
    m_buffer = std::make_shared<ProduceSet>(m_parent);
}

void BrokerProducer::handleResponse(std::shared_ptr<BrokerProducerResponse> response)
{
    if (response->m_err)
    {
        handleError(response->set, response->m_err);
    }
    else
    {
        handleSuccess(response->set, response->res);
    }

    if (m_buffer->Empty())
    {
        rollOver();
    }
}

coev::awaitable<void> BrokerProducer::handleSuccess(std::shared_ptr<ProduceSet> sent, std::shared_ptr<ProduceResponse> response)
{
    std::vector<std::string> retry_topics;

    sent->EachPartition(
        [&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet) -> coev::awaitable<void>
        {
        if (!response) {
            m_parent->returnSuccesses(pSet->m_msgs);
            co_return;
        }
        
        auto block = response->GetBlock(topic, partition);
        if (!block) {
            m_parent->returnErrors(pSet->m_msgs, ErrIncompleteResponse);
            co_return;
        }
        
        switch (block->m_err) {
        case ErrNoError:
            if (m_parent->m_conf->Version.IsAtLeast(V0_10_0_0) && block->m_timestamp !=  std::chrono:: system_clock::time_point{}) {
                for (auto& msg : pSet->m_msgs) {
                    msg->m_timestamp = block->m_timestamp;
                }
            }
            for (size_t i = 0; i < pSet->m_msgs.size(); ++i) {
                pSet->m_msgs[i]->m_offset = block->m_offset + i;
            }
            m_parent->returnSuccesses(pSet->m_msgs);
            break;
        case ErrDuplicateSequenceNumber:
            m_parent->returnSuccesses(pSet->m_msgs);
            break;
        default:
            if (m_parent->m_conf->Producer.Retry.Max <= 0) {
                m_parent->abandonBrokerConnection(m_broker);
                m_parent->returnErrors(pSet->m_msgs, block->m_err);
            } else {
                retry_topics.push_back(topic);
            }
            break;
        } });

    if (!retry_topics.empty())
    {
        if (m_parent->m_conf->Producer.Idempotent)
        {
            int err = co_await m_parent->m_client->RefreshMetadata(retry_topics);
            if (err != 0)
            {
            }
        }

        sent->EachPartition(
            [&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
            {
            auto block = response->GetBlock(topic, partition);
            if (!block) {
                return;
            }
            
            switch (block->m_err) {
            case ErrInvalidMessage:
            case ErrUnknownTopicOrPartition:
            case ErrLeaderNotAvailable:
            case ErrNotLeaderForPartition:
            case ErrRequestTimedOut:
            case ErrNotEnoughReplicas:
            case ErrNotEnoughReplicasAfterAppend:
            case ErrKafkaStorageError:
                {
                    m_current_retries[topic] = std::map<int32_t,KError>();                
                    m_current_retries[topic][partition] = block->m_err;
                    if (m_parent->m_conf->Producer.Idempotent) 
                    {
                        co_start << m_parent->retryBatch(topic, partition, pSet, block->m_err);            
                    } 
                    else
                     {
                        m_parent->retryMessages(pSet->m_msgs, block->m_err);
                    }
                    auto dropped = m_buffer->DropPartition(topic, partition);
                    m_parent->retryMessages(dropped, block->m_err);
                }
                break;
            
            } });
    }
}

void BrokerProducer::handleError(std::shared_ptr<ProduceSet> _sent, KError err)
{
    auto target = KErrorToString(err);
    if (target.size())
    {
        _sent->EachPartition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                             { m_parent->returnErrors(pSet->m_msgs, err); });
    }
    else
    {
        m_parent->abandonBrokerConnection(m_broker);
        m_broker->Close();
        m_closing = err;

        _sent->EachPartition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                             { m_parent->retryMessages(pSet->m_msgs, err); });

        m_buffer->EachPartition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                                { m_parent->retryMessages(pSet->m_msgs, err); });

        rollOver();
    }
}
