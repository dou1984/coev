#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <chrono>
#include <cstdint>
#include "config.h"
#include "client.h"
#include "broker.h"
#include "record.h"
#include "consumer_error.h"
#include "fetch_response.h"

const int32_t invalidLeaderEpoch = -1;
const int32_t invalidPreferredReplicaID = -1;

struct PartitionConsumer;
struct BrokerConsumer;
struct RecordHeader;

struct IConsumer
{
    virtual ~IConsumer() = default;

    virtual int Topics(std::vector<std::string> &) = 0;
    virtual coev::awaitable<int> Partitions(const std::string &topic, std::vector<int32_t> &) = 0;
    virtual coev::awaitable<int> Close() = 0;
    virtual std::map<std::string, std::map<int32_t, int64_t>> HighWaterMarks() = 0;
    virtual void Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions) = 0;
    virtual void Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions) = 0;
    virtual void PauseAll() = 0;
    virtual void ResumeAll() = 0;
};

struct Consumer : IConsumer, std::enable_shared_from_this<Consumer>
{

    int Topics(std::vector<std::string> &out);
    coev::awaitable<int> Partitions(const std::string &topic, std::vector<int32_t> &out);
    coev::awaitable<int> Close();
    coev::awaitable<int> ConsumePartition(const std::string &topic, int32_t partition, int64_t offset, std::shared_ptr<PartitionConsumer> &child);

    std::map<std::string, std::map<int32_t, int64_t>> HighWaterMarks();
    void Pause(const std::map<std::string, std::vector<int32_t>> &topicPartitions);
    void Resume(const std::map<std::string, std::vector<int32_t>> &topicPartitions);
    void PauseAll();
    void ResumeAll();

    int AddChild(const std::shared_ptr<PartitionConsumer> &child);
    void RemoveChild(const std::shared_ptr<PartitionConsumer> &child);
    std::shared_ptr<BrokerConsumer> RefBrokerConsumer(std::shared_ptr<Broker> broker);
    void AbandonBrokerConsumer(std::shared_ptr<BrokerConsumer> brokerWorker);

    std::shared_ptr<Config> m_conf;
    std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionConsumer>>> m_children;
    std::map<int32_t, std::shared_ptr<BrokerConsumer>> m_broker_consumers;
    std::shared_ptr<Client> m_client;
    coev::co_task m_task;
};

int NewConsumer(const std::shared_ptr<Client> &client, std::shared_ptr<IConsumer> &consumer_);
int NewConsumerFromClient(const std::shared_ptr<Client> &client, std::shared_ptr<IConsumer> &consumer_);
coev::awaitable<int> NewConsumer(const std::vector<std::string> &addrs, const std::shared_ptr<Config> &config, std::shared_ptr<Consumer> &consumer_);
