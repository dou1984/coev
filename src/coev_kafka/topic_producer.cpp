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
    // m_task << dispatch();
}
TopicProducer::~TopicProducer()
{
    m_parent->m_topic_producer.erase(m_topic);
}
coev::awaitable<void> TopicProducer::dispatch()
{
    while (true)
    {
        std::shared_ptr<ProducerMessage> msg;
        co_await m_input.get(msg);
        if (msg->m_retries == 0)
        {
            auto err = co_await partition_message(msg);
            if (err != 0)
            {
                m_parent->return_error(msg, err);
                continue;
            }
        }

        auto it = m_handlers.find(msg->m_partition);
        if (it == m_handlers.end())
        {
            it = m_handlers.emplace(msg->m_partition, std::make_shared<PartitionProducer>(m_parent, msg->m_topic, msg->m_partition)).first;
        }
        it->second->m_input.set(msg);
    }
}

coev::awaitable<int> TopicProducer::partition_message(std::shared_ptr<ProducerMessage> &msg)
{

    bool requires_consistency = false;
    if (auto dcp = dynamic_cast<DynamicConsistencyPartitioner *>(m_partitioner.get()))
    {
        requires_consistency = dcp->message_requires_consistency(msg);
    }
    else
    {
        requires_consistency = m_partitioner->requires_consistency();
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
    err = m_partitioner->partition(msg, num_partitions, choice);
    if (choice < 0 || choice >= num_partitions)
    {
        co_return ErrInvalidPartition;
    }

    msg->m_partition = partitions[choice];
    co_return 0;
}