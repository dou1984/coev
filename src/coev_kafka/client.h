#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <atomic>
#include <shared_mutex>
#include <coev/coev.h>
#include <cstdint>
#include "config.h"
#include "metadata_response.h"
#include "metadata_response.h"
#include "init_producer_id_response.h"
#include "find_coordinator_response.h"
#include "offset_request.h"
#include "coordinator_type.h"
#include "metadata_refresher.h"
#include "topic_type.h"

struct Broker;
struct PartitionMetadata;

struct Client
{
    Client(std::shared_ptr<Config> conf);
    virtual ~Client();

    std::deque<std::shared_ptr<Broker>> Brokers();
    std::shared_ptr<Config> GetConfig();
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
    coev::awaitable<int> InitProducerID(InitProducerIDResponse &response);
    coev::awaitable<int> LeastLoadedBroker(std::shared_ptr<Broker> &);
    coev::awaitable<int> MetadataRefresh(const std::vector<std::string> &);
    coev::awaitable<int> BackgroundMetadataUpdater();
    coev::awaitable<int> RefreshMetadata();
    coev::awaitable<int> RetryRefreshMetadata(const std::vector<std::string> &topics, int attempts_remaining, std::chrono::steady_clock::time_point deadline, int err);
    coev::awaitable<int> TryRefreshMetadata(const std::vector<std::string> &topics, int attempts_remaining, std::chrono::steady_clock::time_point deadline);
    coev::awaitable<int> FindCoordinator(const std::string &coordinatorKey, CoordinatorType coordinatorType, int attempts_remaining, FindCoordinatorResponse &response);
    coev::awaitable<int> ResolveCanonicalNames(const std::vector<std::string> &addrs, std::vector<std::string> &);

    int Close();
    bool Closed();
    int Topics(std::vector<std::string> &topics);
    bool PartitionNotReadable(const std::string &topic, int32_t partition);
    int MetadataTopics(std::vector<std::string> &topics);
    bool UpdateMetadata(std::shared_ptr<MetadataResponse> data, bool allKnownMetaData);
    void RandomizeSeedBrokers(const std::vector<std::string> &addrs);
    void UpdateBroker(const std::vector<std::shared_ptr<Broker>> &brokers);
    void RegisterBroker(std::shared_ptr<Broker> broker);
    void DeregisterBroker(std::shared_ptr<Broker> broker);
    void ResurrectDeadBrokers();
    void DeregisterController();
    std::chrono::milliseconds ComputeBackoff(int attempts_remaining);
    coev::awaitable<int> _GetPartitions(const std::string &topic, int64_t pt, std::vector<int32_t> &partitions);
    coev::awaitable<int> _GetReplicas(const std::string &topic, int32_t partitionID, std::shared_ptr<PartitionMetadata> &out);
    coev::awaitable<int> _GetOffset(const std::string &topic, int32_t partitionID, int64_t timestamp, int64_t &offset);
    coev::awaitable<int> _RefreshMetadata(const std::vector<std::string> &topics);
    coev::awaitable<int> _CachedLeader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &broker_, int32_t &leaderEpoch);
    std::shared_ptr<PartitionMetadata> _CachedMetadata(const std::string &topic, int32_t partitionID);
    std::vector<int32_t> _CachedPartitions(const std::string &topic, int64_t partitionSet);
    std::vector<int32_t> _SetPartitionCache(const std::string &topic, int64_t partitionSet);
    std::shared_ptr<Broker> _CachedCoordinator(const std::string &consumerGroup);
    std::shared_ptr<Broker> _CachedTransactionCoordinator(const std::string &transactionID);
    std::shared_ptr<Broker> _CachedController();

    MetadataRefresher m_metadata_refresher;
    std::atomic<int64_t> m_update_metadata_ms;
    std::shared_ptr<Config> m_conf;
    int32_t m_controller_id = INVALID;
    std::deque<std::shared_ptr<Broker>> m_seed_brokers;
    std::deque<std::shared_ptr<Broker>> m_dead_seeds;
    std::map<int32_t, std::shared_ptr<Broker>> m_brokers;
    std::map<topic_t, PartitionMetadata> m_metadata;
    std::map<std::string, bool> m_metadata_topics;
    std::map<std::string, int32_t> m_coordinators;
    std::map<std::string, int32_t> m_transaction_coordinators;
    std::map<std::string, std::array<std::vector<int32_t>, MaxPartitionIndex>> m_cached_partitions_results;

    coev::co_task m_task;
};

coev::awaitable<int> NewClient(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<Client> &);