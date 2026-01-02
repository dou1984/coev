#include "broker_producer.h"

BrokerProducer::BrokerProducer(std::shared_ptr<AsyncProducer> parent, std::shared_ptr<Broker> broker)
    : parent_(parent), broker_(broker), timer_(parent_->conf_->Producer.Flush.Frequency.count() / 1000, 0), timer_fired_(false)
{

    co_start << run();
}

coev::awaitable<void> BrokerProducer::run()
{
    coev::co_channel<std::shared_ptr<ProduceSet>> output;
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        if ((msg = co_await input_.get()))
        {
            shutdown();
            co_return;
        }

        if (msg->Flags & FlagSet::Syn)
        {
            current_retries_[msg->Topic][msg->Partition] = ErrNoError;
            parent_->in_flight_--;
            co_return;
        }

        auto reason = needsRetry(msg);
        if (reason)
        {
            parent_->retryMessage(msg, reason);

            if (!closing_ && (msg->Flags & FlagSet::Fin))
            {
                current_retries_[msg->Topic].erase(msg->Partition);
            }
            continue;
        }

        if (msg->Flags & FlagSet::Fin)
        {
            parent_->retryMessage(msg, ErrShuttingDown);
            continue;
        }

        if (buffer_->WouldOverflow(msg))
        {
            auto err = waitForSpace(msg, false);
            if (err != 0)
            {
                parent_->retryMessage(msg, (KError)err);
                continue;
            }
        }

        if (parent_->txnmgr_->producerID_ != noProducerID && buffer_->producerEpoch != msg->ProducerEpoch)
        {
            int err = waitForSpace(msg, true);
            if (err != 0)
            {
                parent_->retryMessage(msg, (KError)err);
                continue;
            }
        }

        int err = buffer_->Add(msg);
        if (err != 0)
        {
            parent_->returnError(msg, err);
            continue;
        }

        if (parent_->conf_->Producer.Flush.Frequency > std::chrono::milliseconds(0) && !timer_.is_active())
        {
            timer_.reset(parent_->conf_->Producer.Flush.Frequency.count() / 1000, 0);
        }

        std::shared_ptr<BrokerProducerResponse> response;
        if (responses_.try_get(response))
        {
            handleResponse(response);
        }

        if (timer_fired_ || buffer_->ReadyToFlush())
        {
            output_.set(buffer_);
        }
    }
}

coev::awaitable<void> BrokerProducer::shutdown()
{
    while (!buffer_->Empty())
    {
        std::shared_ptr<BrokerProducerResponse> response;
        if (responses_.try_get(response))
        {
            handleResponse(response);
        }
        output_.set(buffer_);
        rollOver();
    }

    std::shared_ptr<BrokerProducerResponse> response;
    while ((response = co_await responses_.get()))
    {
        handleResponse(response);
    }
}

KError BrokerProducer::needsRetry(std::shared_ptr<ProducerMessage> msg)
{
    if (closing_ != ErrNoError)
    {
        return closing_;
    }

    auto it = current_retries_.find(msg->Topic);
    if (it != current_retries_.end())
    {
        auto pit = it->second.find(msg->Partition);
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
        if (responses_.try_get(response))
        {
            handleResponse(response);

            auto reason = needsRetry(msg);
            if (reason)
            {
                return -1;
            }
            else if (!buffer_->WouldOverflow(msg) && !force_rollover)
            {
                return 0;
            }
        }
        output_.set(buffer_);
        rollOver();
        return 0;
    }
}

void BrokerProducer::rollOver()
{
    timer_.stop();
    timer_fired_ = false;
    buffer_ = std::make_shared<ProduceSet>(parent_);
}

void BrokerProducer::handleResponse(std::shared_ptr<BrokerProducerResponse> response)
{
    if (response->err)
    {
        handleError(response->set, response->err);
    }
    else
    {
        handleSuccess(response->set, response->res);
    }

    if (buffer_->Empty())
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
            parent_->returnSuccesses(pSet->msgs);
            co_return;
        }
        
        auto block = response->GetBlock(topic, partition);
        if (!block) {
            parent_->returnErrors(pSet->msgs, ErrIncompleteResponse);
            co_return;
        }
        
        switch (block->Err) {
        case ErrNoError:
            if (parent_->conf_->Version.IsAtLeast(V0_10_0_0) && block->Timestamp !=  std::chrono:: system_clock::time_point{}) {
                for (auto& msg : pSet->msgs) {
                    msg->Timestamp = block->Timestamp;
                }
            }
            for (size_t i = 0; i < pSet->msgs.size(); ++i) {
                pSet->msgs[i]->Offset = block->Offset + i;
            }
            parent_->returnSuccesses(pSet->msgs);
            break;
        case ErrDuplicateSequenceNumber:
            parent_->returnSuccesses(pSet->msgs);
            break;
        default:
            if (parent_->conf_->Producer.Retry.Max <= 0) {
                parent_->abandonBrokerConnection(broker_);
                parent_->returnErrors(pSet->msgs, block->Err);
            } else {
                retry_topics.push_back(topic);
            }
            break;
        } });

    if (!retry_topics.empty())
    {
        if (parent_->conf_->Producer.Idempotent)
        {
            int err = co_await parent_->client_->RefreshMetadata(retry_topics);
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
            
            switch (block->Err) {
            case ErrInvalidMessage:
            case ErrUnknownTopicOrPartition:
            case ErrLeaderNotAvailable:
            case ErrNotLeaderForPartition:
            case ErrRequestTimedOut:
            case ErrNotEnoughReplicas:
            case ErrNotEnoughReplicasAfterAppend:
            case ErrKafkaStorageError:
                {
                    current_retries_[topic] = std::map<int32_t,KError>();                
                    current_retries_[topic][partition] = block->Err;
                    if (parent_->conf_->Producer.Idempotent) 
                    {
                        co_start << parent_->retryBatch(topic, partition, pSet, block->Err);            
                    } 
                    else
                     {
                        parent_->retryMessages(pSet->msgs, block->Err);
                    }
                    auto dropped = buffer_->DropPartition(topic, partition);
                    parent_->retryMessages(dropped, block->Err);
                }
                break;
            
            } });
    }
}

void BrokerProducer::handleError(std::shared_ptr<ProduceSet> sent, KError err)
{
    auto target = KErrorToString(err);
    if (target.size())
    {
        sent->EachPartition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                            { parent_->returnErrors(pSet->msgs, err); });
    }
    else
    {
        parent_->abandonBrokerConnection(broker_);
        broker_->Close();
        closing_ = err;

        sent->EachPartition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                            { parent_->retryMessages(pSet->msgs, err); });

        buffer_->EachPartition([&](const std::string &topic, int32_t partition, std::shared_ptr<PartitionSet> pSet)
                               { parent_->retryMessages(pSet->msgs, err); });

        rollOver();
    }
}
