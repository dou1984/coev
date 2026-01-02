#include "topic_producer.h"
#include "broker.h"
#include "producer_message.h"
#include "partitioner.h"
#include "partition_producer.h"

TopicProducer::TopicProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic)
    : parent_(parent), topic_(topic)
{
    partitioner_ = parent_->conf_->Producer.Partitioner(topic);
}

coev::awaitable<void> TopicProducer::dispatch()
{

    while (true)
    {
        std::shared_ptr<ProducerMessage> msg = co_await input_.get();
        if (msg->Retries == 0)
        {
            int err = co_await partitionMessage(msg);
            if (err != 0)
            {
                parent_->returnError(msg, err);
                continue;
            }
        }

        auto &handler = handlers_[msg->Partition];
        if (auto it = handlers_.find(msg->Partition); it != handlers_.end())
        {
            it->second.set(msg);
        }
        else
        {
            auto pp = std::make_shared<PartitionProducer>(parent_, msg->Topic, msg->Partition);
            co_start << pp->Dispatch();
            handler.set(msg);
        }
    }
}

coev::awaitable<int> TopicProducer::partitionMessage(std::shared_ptr<ProducerMessage> msg)
{
    std::vector<int32_t> partitions;

    bool requires_consistency = false;
    if (auto ep = dynamic_cast<DynamicConsistencyPartitioner *>(partitioner_.get()))
    {
        requires_consistency = ep->MessageRequiresConsistency(msg);
    }
    else
    {
        requires_consistency = partitioner_->RequiresConsistency();
    }

    int err = 0;
    if (requires_consistency)
    {
        err = co_await parent_->client_->Partitions(msg->Topic, partitions);
    }
    else
    {
        err = co_await parent_->client_->WritablePartitions(msg->Topic, partitions);
    }

    if (err != 0)
    {
        co_return err;
    }

    int32_t num_partitions = partitions.size();
    if (num_partitions == 0)
    {
        co_return ErrLeaderNotAvailable;
    }

    int32_t choice;
    err = partitioner_->Partition(msg, num_partitions, choice);
    if (choice < 0 || choice >= num_partitions)
    {
        co_return ErrInvalidPartition;
    }

    msg->Partition = partitions[choice];
    co_return 0;
}