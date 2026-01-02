
#include <functional>
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <queue>
#include "errors.h"
#include "sleep_for.h"
#include "metrics.h"
#include "response_header.h"
#include "interceptors.h"
#include "client.h"
#include "consumer.h"
#include "nop_closer_client.h"
#include "partition_consumer.h"
#include "broker_consumer.h"

coev::awaitable<int> Consumer::Close()
{
    metricRegistry->UnregisterAll();
    co_return client->Close();
}

int Consumer::Topics(std::vector<std::string> &out_topics)
{
    return client->Topics(out_topics);
}

coev::awaitable<int> Consumer::Partitions(const std::string &topic, std::vector<int32_t> &out_partitions)
{
    return client->Partitions(topic, out_partitions);
}

std::map<std::string, std::map<int32_t, int64_t>> Consumer::HighWaterMarks()
{
    std::lock_guard<std::mutex> lock(lock_);
    std::map<std::string, std::map<int32_t, int64_t>> hwms;
    for (auto &topic_pair : children)
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
    std::lock_guard<std::mutex> lock(lock_);
    for (auto &tp : topicPartitions)
    {
        auto &topic = tp.first;
        auto &partitions = tp.second;
        auto it_topic = children.find(topic);
        if (it_topic != children.end())
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
    std::lock_guard<std::mutex> lock(lock_);
    for (auto &tp : topicPartitions)
    {
        auto &topic = tp.first;
        auto &partitions = tp.second;
        auto it_topic = children.find(topic);
        if (it_topic != children.end())
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
    std::lock_guard<std::mutex> lock(lock_);
    for (auto &topic_pair : children)
    {
        for (auto &pc_pair : topic_pair.second)
        {
            pc_pair.second->Pause();
        }
    }
}

void Consumer::ResumeAll()
{
    std::lock_guard<std::mutex> lock(lock_);
    for (auto &topic_pair : children)
    {
        for (auto &pc_pair : topic_pair.second)
        {
            pc_pair.second->Resume();
        }
    }
}

int Consumer::addChild(const std::shared_ptr<PartitionConsumer> &child)
{
    std::lock_guard<std::mutex> lock(lock_);
    auto &topicChildren = children[child->topic];
    if (topicChildren.find(child->partition) != topicChildren.end())
    {
        return ErrTopicPartitionConsumed;
    }
    topicChildren[child->partition] = child;
    return 0;
}

void Consumer::removeChild(const std::shared_ptr<PartitionConsumer> &child)
{
    std::lock_guard<std::mutex> lock(lock_);
    auto it = children.find(child->topic);
    if (it != children.end())
    {
        it->second.erase(child->partition);
    }
}

std::shared_ptr<BrokerConsumer> Consumer::refBrokerConsumer(std::shared_ptr<Broker> broker)
{
    std::lock_guard<std::mutex> lock(lock_);
    auto it = brokerConsumers.find(broker);
    if (it == brokerConsumers.end())
    {
        auto bc = NewBrokerConsumer(shared_from_this(), broker);
        brokerConsumers[broker] = bc;
        it = brokerConsumers.find(broker);
    }
    it->second->refs++;
    return it->second;
}

void Consumer::unrefBrokerConsumer(std::shared_ptr<BrokerConsumer> brokerWorker)
{
    std::lock_guard<std::mutex> lock(lock_);
    brokerWorker->refs--;
    if (brokerWorker->refs == 0)
    {
        auto it = brokerConsumers.find(brokerWorker->broker);
        if (it != brokerConsumers.end() && it->second.get() == brokerWorker.get())
        {
            brokerConsumers.erase(it);
        }
    }
}
coev::awaitable<int> Consumer::ConsumePartition(const std::string &topic, int32_t partition, int64_t offset, std::shared_ptr<PartitionConsumer> &child)
{
    child = std::make_shared<PartitionConsumer>();
    child->consumer = shared_from_this();
    child->conf = conf;
    child->topic = topic;
    child->partition = partition;

    child->leaderEpoch = invalidLeaderEpoch;
    child->preferredReadReplica = invalidPreferredReplicaID;
    child->fetchSize = conf->Consumer.Fetch.DefaultVal;

    int err = co_await child->ChooseStartingOffset(offset);
    if (err != 0)
    {
        co_return err;
    }

    std::shared_ptr<Broker> leader;
    int32_t epoch;
    err = co_await client->LeaderAndEpoch(topic, partition, leader, epoch);
    if (err != 0)
    {
        co_return err;
    }

    err = addChild(child);
    if (err != 0)
    {
        co_return err;
    }

    task_ << child->Dispatcher();
    task_ << child->ResponseFeeder();

    child->leaderEpoch = epoch;
    child->broker = refBrokerConsumer(leader);
    child->broker->input.set(child);

    co_return 0;
}

void Consumer::abandonBrokerConsumer(std::shared_ptr<BrokerConsumer> brokerWorker)
{
    std::lock_guard<std::mutex> lock(lock_);
    brokerConsumers.erase(brokerWorker->broker);
}

int NewConsumer(const std::shared_ptr<IClient> &client, std::shared_ptr<IConsumer> &consumer_)
{
    if (client->Closed())
    {
        return ErrClosedClient;
    }

    auto consumer = std::make_shared<Consumer>();
    consumer->client = client;
    consumer->conf = client->GetConfig();
    consumer->metricRegistry = NewCleanupRegistry(consumer->conf->MetricRegistry);
    consumer_ = consumer;
    return 0;
}
int NewConsumerFromClient(const std::shared_ptr<IClient> &client, std::shared_ptr<IConsumer> &consumer)
{
    auto cli = std::make_shared<NopCloserClient>(client);
    return NewConsumer(cli, consumer);
}
coev::awaitable<int> NewConsumer(const std::vector<std::string> &addrs, const std::shared_ptr<Config> &config, std::shared_ptr<IConsumer> &consumer_)
{
    std::shared_ptr<Client> client;
    auto err = co_await NewClient(addrs, config, client);
    if (err != 0)
    {
        co_return err;
    }
    err = NewConsumer(client, consumer_);
    if (err != 0)
    {
        co_return err;
    }
    co_return err;
}
