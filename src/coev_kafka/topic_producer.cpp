#include "topic_producer.h"
#include "broker.h"
#include "producer_message.h"
#include "partitioner.h"
#include "partition_producer.h"

TopicProducer::TopicProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic)
    : m_parent(parent), m_topic(topic)
{
    m_partitioner = m_parent->m_conf->Producer.Partitioner(topic);
}

coev::awaitable<void> TopicProducer::dispatch()
{

    while (true)
    {
        std::shared_ptr<ProducerMessage> msg = co_await m_input.get();
        if (msg->m_retries == 0)
        {
            int err = co_await partitionMessage(msg);
            if (err != 0)
            {
                m_parent->returnError(msg, err);
                continue;
            }
        }

        auto &handler = m_handlers[msg->m_partition];
        if (auto it = m_handlers.find(msg->m_partition); it != m_handlers.end())
        {
            it->second.set(msg);
        }
        else
        {
            auto pp = std::make_shared<PartitionProducer>(m_parent, msg->m_topic, msg->m_partition);
            co_start << pp->Dispatch();
            handler.set(msg);
        }
    }
}

coev::awaitable<int> TopicProducer::partitionMessage(std::shared_ptr<ProducerMessage> msg)
{
    std::vector<int32_t> partitions;

    bool requires_consistency = false;
    if (auto ep = dynamic_cast<DynamicConsistencyPartitioner *>(m_partitioner.get()))
    {
        requires_consistency = ep->MessageRequiresConsistency(msg);
    }
    else
    {
        requires_consistency = m_partitioner->RequiresConsistency();
    }

    int err = 0;
    if (requires_consistency)
    {
        err = co_await m_parent->m_client->Partitions(msg->m_topic, partitions);
    }
    else
    {
        err = co_await m_parent->m_client->WritablePartitions(msg->m_topic, partitions);
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
    err = m_partitioner->Partition(msg, num_partitions, choice);
    if (choice < 0 || choice >= num_partitions)
    {
        co_return ErrInvalidPartition;
    }

    msg->m_partition = partitions[choice];
    co_return 0;
}