#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <shared_mutex>
#include <coev.h>
#include <cstdint>
#include "config.h"
#include "metadata_response.h"
#include "metadata_response.h"
#include "init_producer_id_response.h"
#include "find_coordinator_response.h"
#include "offset_request.h"
#include "coordinator_type.h"

using PartitionType = int8_t;

struct Broker;
struct PartitionMetadata;

struct IClient
{
    virtual ~IClient() = default;

    virtual std::shared_ptr<Config> GetConfig() = 0;
    virtual coev::awaitable<int> Controller(std::shared_ptr<Broker> &broker) = 0;
    virtual coev::awaitable<int> RefreshController(std::shared_ptr<Broker> &broker) = 0;
    virtual std::vector<std::shared_ptr<Broker>> Brokers() = 0;
    virtual coev::awaitable<int> GetBroker(int32_t brokerID, std::shared_ptr<Broker> &broker) = 0;
    virtual int Topics(std::vector<std::string> &topics) = 0;
    virtual coev::awaitable<int> Partitions(const std::string &topic, std::vector<int32_t> &partitions) = 0;
    virtual coev::awaitable<int> WritablePartitions(const std::string &topic, std::vector<int32_t> &partitions) = 0;
    virtual coev::awaitable<int> Leader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader) = 0;
    virtual coev::awaitable<int> LeaderAndEpoch(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader, int32_t &epoch) = 0;
    virtual coev::awaitable<int> Replicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &replicas) = 0;
    virtual coev::awaitable<int> InSyncReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &isr) = 0;
    virtual coev::awaitable<int> OfflineReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &offline) = 0;
    virtual coev::awaitable<int> RefreshBrokers(const std::vector<std::string> &addrs) = 0;
    virtual coev::awaitable<int> RefreshMetadata(const std::vector<std::string> &topics) = 0;
    virtual coev::awaitable<int> GetOffset(const std::string &topic, int32_t partitionID, int64_t time, int64_t &offset) = 0;
    virtual coev::awaitable<int> GetCoordinator(const std::string &consumerGroup, std::shared_ptr<Broker> &coordinator) = 0;
    virtual coev::awaitable<int> RefreshCoordinator(const std::string &consumerGroup) = 0;
    virtual coev::awaitable<int> TransactionCoordinator(const std::string &transactionID, std::shared_ptr<Broker> &coordinator) = 0;
    virtual coev::awaitable<int> RefreshTransactionCoordinator(const std::string &transactionID) = 0;
    virtual coev::awaitable<int> InitProducerID(std::shared_ptr<InitProducerIDResponse> &response) = 0;
    virtual coev::awaitable<int> LeastLoadedBroker(std::shared_ptr<Broker> &) = 0;
    virtual bool PartitionNotReadable(const std::string &topic, int32_t partition) = 0;
    virtual int Close() = 0;
    virtual bool Closed() = 0;
};

struct Client : IClient
{

    Client(std::shared_ptr<Config> conf);
    ~Client() = default;

    std::shared_ptr<Config> GetConfig();
    coev::awaitable<int> Controller(std::shared_ptr<Broker> &broker);
    coev::awaitable<int> RefreshController(std::shared_ptr<Broker> &broker);
    std::vector<std::shared_ptr<Broker>> Brokers();
    coev::awaitable<int> GetBroker(int32_t brokerID, std::shared_ptr<Broker> &broker);
    int Topics(std::vector<std::string> &topics);
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
    coev::awaitable<int> metadataRefresh(const std::vector<std::string> &);

    int Close();
    bool Closed();

    int MetadataTopics(std::vector<std::string> &topics);
    coev::awaitable<int> backgroundMetadataUpdater();
    coev::awaitable<int> refreshMetadata();
    coev::awaitable<int> retryRefreshMetadata(const std::vector<std::string> &topics, int attemptsRemaining, std::chrono::steady_clock::time_point deadline, int err);
    coev::awaitable<int> tryRefreshMetadata(const std::vector<std::string> &topics, int attemptsRemaining, std::chrono::steady_clock::time_point deadline);
    coev::awaitable<int> findCoordinator(const std::string &coordinatorKey, CoordinatorType coordinatorType, int attemptsRemaining, std::shared_ptr<FindCoordinatorResponse> &response);
    coev::awaitable<int> resolveCanonicalNames(const std::vector<std::string> &addrs, std::vector<std::string> &);
    bool updateMetadata(std::shared_ptr<MetadataResponse> data, bool allKnownMetaData);
    void randomizeSeedBrokers(const std::vector<std::string> &addrs);
    void updateBroker(const std::vector<std::shared_ptr<Broker>> &brokers);
    void registerBroker(std::shared_ptr<Broker> broker);
    void deregisterBroker(std::shared_ptr<Broker> broker);
    void resurrectDeadBrokers();
    void deregisterController();
    std::chrono::milliseconds ComputeBackoff(int attemptsRemaining);

    coev::awaitable<int> getPartitions(const std::string &topic, int64_t pt, std::vector<int32_t> &partitions);
    coev::awaitable<int> getReplicas(const std::string &topic, int32_t partitionID, std::shared_ptr<PartitionMetadata> &out);
    coev::awaitable<int> getOffset(const std::string &topic, int32_t partitionID, int64_t timestamp, int64_t &offset);
    std::shared_ptr<PartitionMetadata> cachedMetadata(const std::string &topic, int32_t partitionID);
    std::vector<int32_t> cachedPartitions(const std::string &topic, int64_t partitionSet);
    std::vector<int32_t> setPartitionCache(const std::string &topic, int64_t partitionSet);
    coev::awaitable<int> cachedLeader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &broker_, int32_t &leaderEpoch);
    std::shared_ptr<Broker> cachedCoordinator(const std::string &consumerGroup);
    std::shared_ptr<Broker> cachedTransactionCoordinator(const std::string &transactionID);
    std::shared_ptr<Broker> cachedController();

    std::atomic<int64_t> updateMetadataMs{0};
    std::shared_ptr<Config> conf_;

    std::vector<std::shared_ptr<Broker>> seedBrokers;
    std::vector<std::shared_ptr<Broker>> deadSeeds;
    int32_t controllerID = -1;
    std::map<int32_t, std::shared_ptr<Broker>> brokers;
    std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionMetadata>>> metadata;
    std::map<std::string, bool> metadataTopics;
    std::map<std::string, int32_t> coordinators;
    std::map<std::string, int32_t> transactionCoordinators;
    std::map<std::string, std::array<std::vector<int32_t>, MaxPartitionIndex>> cachedPartitionsResults;
    std::shared_mutex lock;

    coev::co_task task_;
    coev::co_channel<bool> closer;
    coev::co_channel<bool> closed;
};

coev::awaitable<int> NewClient(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<Client> &);