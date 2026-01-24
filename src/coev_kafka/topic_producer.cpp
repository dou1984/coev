#include "topic_producer.h"
#include "broker.h"
#include "producer_message.h"
#include "partitioner.h"
#include "partition_producer.h"

TopicProducer::TopicProducer(std::shared_ptr<AsyncProducer> parent, const std::string &topic)
    : m_parent(parent), m_topic(topic)
{
    assert(m_topic != "");
    m_partitioner = m_parent->m_conf->Producer.Partitioner(topic);
}
TopicProducer::~TopicProducer()
{
    m_parent->m_topic_producer.erase(m_topic);
}
coev::awaitable<int> TopicProducer::dispatch(std::shared_ptr<ProducerMessage> &msg)
{
    if (msg->m_retries == 0)
    {
        auto err = co_await partition_message(msg);
        if (err != 0)
        {
            m_parent->return_error(msg, err);
            co_return INVALID;
        }
    }

    auto it = m_handlers.find(msg->m_partition);
    if (it != m_handlers.end())
    {
        co_await it->second->dispatch(msg);
    }
    else
    {
        auto pp = std::make_shared<PartitionProducer>(m_parent, msg->m_topic, msg->m_partition);
        auto err = co_await pp->init();
        if (err != ErrNoError)
        {
            co_return INVALID;
        }
        m_handlers[msg->m_partition] = pp;
        co_await pp->dispatch(msg);
    }

    co_return 0;
}

coev::awaitable<int> TopicProducer::partition_message(std::shared_ptr<ProducerMessage> msg)
{

    bool requires_consistency = false;
    if (auto dcp = dynamic_cast<DynamicConsistencyPartitioner *>(m_partitioner.get()))
    {
        requires_consistency = dcp->MessageRequiresConsistency(msg);
    }
    else
    {
        requires_consistency = m_partitioner->RequiresConsistency();
    }

    int err = 0;
    std::vector<int32_t> partitions;
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