#pragma once

#include "client.h"

struct NopCloserClient final : IClient
{

    NopCloserClient(std::shared_ptr<IClient> client);
    int Close();

    std::shared_ptr<Config> GetConfig();
    std::vector<std::shared_ptr<Broker>> Brokers();
    int Topics(std::vector<std::string> &topics);
    coev::awaitable<int> Controller(std::shared_ptr<Broker> &broker);
    coev::awaitable<int> RefreshController(std::shared_ptr<Broker> &broker);
    coev::awaitable<int> GetBroker(int32_t brokerID, std::shared_ptr<Broker> &broker);
    coev::awaitable<int> Partitions(const std::string &topic, std::vector<int32_t> &partitions);
    coev::awaitable<int> WritablePartitions(const std::string &topic, std::vector<int32_t> &partitions);
    coev::awaitable<int> Leader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader);
    coev::awaitable<int> LeaderAndEpoch(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader, int32_t &epoch);
    coev::awaitable<int> Replicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &replicas);
    coev::awaitable<int> InSyncReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &isr);
    coev::awaitable<int> OfflineReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &offline);
    coev::awaitable<int> RefreshBrokers(const std::vector<std::string> &addrs);
    coev::awaitable<int> RefreshMetadata(const std::vector<std::string> &topics);
    coev::awaitable<int> GetOffset(const std::string &topic, int32_t partitionID, int64_t time, int64_t &offset);
    coev::awaitable<int> GetCoordinator(const std::string &consumerGroup, std::shared_ptr<Broker> &coordinator);
    coev::awaitable<int> RefreshCoordinator(const std::string &consumerGroup);
    coev::awaitable<int> TransactionCoordinator(const std::string &transactionID, std::shared_ptr<Broker> &coordinator);
    coev::awaitable<int> RefreshTransactionCoordinator(const std::string &transactionID);
    coev::awaitable<int> InitProducerID(std::shared_ptr<InitProducerIDResponse> &response);
    coev::awaitable<int> LeastLoadedBroker(std::shared_ptr<Broker> &);
    bool PartitionNotReadable(const std::string &topic, int32_t partition);
    bool Closed();

    std::shared_ptr<IClient> client_;
};
