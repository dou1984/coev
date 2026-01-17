
#include <functional>
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <queue>
#include "errors.h"
#include "sleep_for.h"
#include "response_header.h"
#include "interceptors.h"
#include "client.h"
#include "consumer.h"
#include "partition_consumer.h"
#include "broker_consumer.h"

coev::awaitable<int> Consumer::Close()
{
    co_return m_client->Close();
}

int Consumer::Topics(std::vector<std::string> &out_topics)
{
    return m_client->Topics(out_topics);
}

coev::awaitable<int> Consumer::Partitions(const std::string &topic, std::vector<int32_t> &out_partitions)
{
    return m_client->Partitions(topic, out_partitions);
}

std::map<std::string, std::map<int32_t, int64_t>> Consumer::HighWaterMarks()
{
    std::lock_guard<std::mutex> lock(m_lock);
    std::map<std::string, std::map<int32_t, int64_t>> hwms;
    for (auto &topic_pair : m_children)
    {
        auto &topic = topic_pair.first;
        auto &p_map = topic_pair.second;
        std::map<int32_t, int64_t> hwm;
        for (auto &part_pair : p_map)
        {
            int32_t partition = part_pair.first;
            auto pc = part_pair.second;
            hwm[partition] = pc->HighWaterMarkOffset();
        }
        hwms[topic] = hwm;
    }
    return hwms;
}

void Consumer::Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions)
{
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto &tp : topicPartitions)
    {
        auto &topic = tp.first;
        auto &partitions = tp.second;
        auto it_topic = m_children.find(topic);
        if (it_topic != m_children.end())
        {
            auto &topic_consumers = it_topic->second;
            for (int32_t partition : partitions)
            {
                auto it_pc = topic_consumers.find(partition);
                if (it_pc != topic_consumers.end())
                {
                    it_pc->second->Pause();
                }
            }
        }
    }
}

void Consumer::Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions)
{
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto &tp : topicPartitions)
    {
        auto &topic = tp.first;
        auto &partitions = tp.second;
        auto it_topic = m_children.find(topic);
        if (it_topic != m_children.end())
        {
            auto &topic_consumers = it_topic->second;
            for (int32_t partition : partitions)
            {
                auto it_pc = topic_consumers.find(partition);
                if (it_pc != topic_consumers.end())
                {
                    it_pc->second->Resume();
                }
            }
        }
    }
}

void Consumer::PauseAll()
{
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto &topic_pair : m_children)
    {
        for (auto &pc_pair : topic_pair.second)
        {
            pc_pair.second->Pause();
        }
    }
}

void Consumer::ResumeAll()
{
    std::lock_guard<std::mutex> lock(m_lock);
    for (auto &topic_pair : m_children)
    {
        for (auto &pc_pair : topic_pair.second)
        {
            pc_pair.second->Resume();
        }
    }
}

int Consumer::addChild(const std::shared_ptr<PartitionConsumer> &child)
{
    std::lock_guard<std::mutex> lock(m_lock);
    auto &topicChildren = m_children[child->m_topic];
    if (topicChildren.find(child->m_partition) != topicChildren.end())
    {
        return ErrTopicPartitionConsumed;
    }
    topicChildren[child->m_partition] = child;
    return 0;
}

void Consumer::removeChild(const std::shared_ptr<PartitionConsumer> &child)
{
    std::lock_guard<std::mutex> lock(m_lock);
    auto it = m_children.find(child->m_topic);
    if (it != m_children.end())
    {
        it->second.erase(child->m_partition);
    }
}

std::shared_ptr<BrokerConsumer> Consumer::refBrokerConsumer(std::shared_ptr<Broker> broker)
{
    std::lock_guard<std::mutex> lock(m_lock);
    auto it = m_broker_consumers.find(broker);
    if (it == m_broker_consumers.end())
    {
        auto bc = NewBrokerConsumer(shared_from_this(), broker);
        m_broker_consumers[broker] = bc;
        it = m_broker_consumers.find(broker);
    }
    it->second->m_refs++;
    return it->second;
}

void Consumer::unrefBrokerConsumer(std::shared_ptr<BrokerConsumer> brokerWorker)
{
    std::lock_guard<std::mutex> lock(m_lock);
    brokerWorker->m_refs--;
    if (brokerWorker->m_refs == 0)
    {
        auto it = m_broker_consumers.find(brokerWorker->m_broker);
        if (it != m_broker_consumers.end() && it->second.get() == brokerWorker.get())
        {
            m_broker_consumers.erase(it);
        }
    }
}
coev::awaitable<int> Consumer::ConsumePartition(const std::string &topic, int32_t partition, int64_t offset, std::shared_ptr<PartitionConsumer> &child)
{
    child = std::make_shared<PartitionConsumer>();
    child->m_consumer = shared_from_this();
    child->m_conf = m_conf;
    child->m_topic = topic;
    child->m_partition = partition;

    child->m_leader_epoch = invalidLeaderEpoch;
    child->m_preferred_read_replica = invalidPreferredReplicaID;
    child->m_fetch_size = m_conf->Consumer.Fetch.DefaultVal;

    int err = co_await child->ChooseStartingOffset(offset);
    if (err != 0)
    {
        co_return err;
    }

    std::shared_ptr<Broker> leader;
    int32_t epoch;
    err = co_await m_client->LeaderAndEpoch(topic, partition, leader, epoch);
    if (err != 0)
    {
        co_return err;
    }

    err = addChild(child);
    if (err != 0)
    {
        co_return err;
    }

    m_task << child->Dispatcher();
    m_task << child->ResponseFeeder();

    child->m_leader_epoch = epoch;
    child->m_broker = refBrokerConsumer(leader);
    child->m_broker->m_input.set(child);

    co_return 0;
}

void Consumer::abandonBrokerConsumer(std::shared_ptr<BrokerConsumer> brokerWorker)
{
    std::lock_guard<std::mutex> lock(m_lock);
    m_broker_consumers.erase(brokerWorker->m_broker);
}

int NewConsumer(const std::shared_ptr<Client> &client, std::shared_ptr<Consumer> &consumer_)
{
    if (client->Closed())
    {
        return ErrClosedClient;
    }

    auto consumer = std::make_shared<Consumer>();
    consumer->m_client = client;
    consumer->m_conf = client->GetConfig();
    consumer_ = consumer;
    return 0;
}
int NewConsumerFromClient(const std::shared_ptr<Client> &client, std::shared_ptr<Consumer> &consumer)
{
    return NewConsumer(client, consumer);
}
coev::awaitable<int> NewConsumer(const std::vector<std::string> &addrs, const std::shared_ptr<Config> &config, std::shared_ptr<Consumer> &consumer_)
{
    std::shared_ptr<Client> client;
    auto err = co_await NewClient(addrs, config, client);
    if (err != 0)
    {
        co_return err;
    }
    co_return NewConsumerFromClient(client, consumer_);
}
