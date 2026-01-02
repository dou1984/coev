
#include <random>
#include <algorithm>
#include <chrono>
#include <functional>
#include "logger.h"
#include "utils.h"
#include "client.h"
#include "broker.h"
#include "errors.h"
#include "sleep_for.h"
#include "compress.h"
#include "offset_request.h"
#include "partition_producer.h"

coev::awaitable<int> NewClient(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<Client> &client_)
{
    DebugLogger::Println("Initializing new client");
    if (conf == nullptr)
    {
        conf = std::make_shared<Config>();
    }
    int err = ErrNoError;
    if ((!conf->Validate()))
    {
        co_return ErrInvalidConfig;
    }
    if (addrs.empty())
    {
        co_return ErrNoConfiguredBrokers;
    }
    if (addrs[0].find(".servicebus.windows.net") != std::string::npos)
    {
        if (conf->Version.IsAtLeast(V1_1_0_0) || !conf->Version.IsAtLeast(V0_11_0_0))
        {
            Logger::Println("Connecting to Azure Event Hubs, forcing version to V1_0_0_0 for compatibility");
            conf->Version = V1_0_0_0;
        }
    }

    client_ = std::make_shared<Client>(conf);
    if (conf->Net.ResolveCanonicalBootstrapServers)
    {
        std::vector<std::string> resolved_addrs;
        err = co_await client_->resolveCanonicalNames(addrs, resolved_addrs);
        if (err != 0)
        {
            co_return err;
        }
        client_->randomizeSeedBrokers(resolved_addrs);
    }
    else
    {
        client_->randomizeSeedBrokers(addrs);
    }

    if (conf->Metadata.Full)
    {
        err = co_await client_->RefreshMetadata({});
        if (err != 0)
        {
            if (err == ErrLeaderNotAvailable || err == ErrReplicaNotAvailable ||
                err == ErrTopicAuthorizationFailed || err == ErrClusterAuthorizationFailed)
            {

                Logger::Println("%d %s", err, KErrorToString(err));
            }
            else
            {
                client_->closed.set(true);
                client_->Close();
                co_return err;
            }
        }
    }

    client_->task_ << client_->backgroundMetadataUpdater();
    DebugLogger::Println("Successfully initialized new client");

    co_return ErrNoError;
}

Client::Client(std::shared_ptr<Config> c) : conf_(c)
{
}

std::shared_ptr<Config> Client::GetConfig()
{
    return conf_;
}

std::vector<std::shared_ptr<Broker>> Client::Brokers()
{
    std::shared_lock<std::shared_mutex> lk(lock);
    std::vector<std::shared_ptr<Broker>> brokers_list;
    for (auto &kv : brokers)
    {
        brokers_list.push_back(kv.second);
    }
    return brokers_list;
}

coev::awaitable<int> Client::GetBroker(int32_t brokerID, std::shared_ptr<Broker> &broker)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto it = brokers.find(brokerID);
    if (it == brokers.end())
    {
        co_return ErrBrokerNotFound;
    }
    broker = it->second;
    auto err = co_await broker->Open(conf_);
    co_return err;
}

coev::awaitable<int> Client::InitProducerID(std::shared_ptr<InitProducerIDResponse> &response)
{
    std::vector<int> brokerErrors;
    std::shared_ptr<Broker> broker;
    int err;
    for (err = co_await LeastLoadedBroker(broker); broker != nullptr; err = co_await LeastLoadedBroker(broker))
    {
        auto request = std::make_shared<InitProducerIDRequest>();
        if (conf_->Version.IsAtLeast(V2_7_0_0))
        {
            request->Version = 4;
        }
        else if (conf_->Version.IsAtLeast(V2_5_0_0))
        {
            request->Version = 3;
        }
        else if (conf_->Version.IsAtLeast(V2_4_0_0))
        {
            request->Version = 2;
        }
        else if (conf_->Version.IsAtLeast(V2_0_0_0))
        {
            request->Version = 1;
        }
        err = co_await broker->InitProducerID(request, response);
        if (err == 0)
        {
            co_return 0;
        }
        else
        {
            Logger::Printf("Client got error from broker %d when issuing InitProducerID : %d\n", broker->ID(), err);
            broker->Close();
            brokerErrors.push_back(err);
            deregisterBroker(broker);
        }
    }
    co_return ErrOutOfBrokers;
}

int Client::Close()
{
    if (Closed())
    {
        Logger::Printf("Close() called on already closed client");
        return ErrClosedClient;
    }
    closer.set(true);

    std::unique_lock<std::shared_mutex> lk(lock);
    DebugLogger::Println("Closing Client");
    for (auto &kv : brokers)
    {
        kv.second->safeAsyncClose();
    }
    for (auto &b : seedBrokers)
    {
        b->safeAsyncClose();
    }
    closed.get();
    brokers.clear();
    metadata.clear();
    metadataTopics.clear();
    return 0;
}

bool Client::Closed()
{
    std::shared_lock<std::shared_mutex> lk(lock);
    return brokers.empty();
}

int Client::Topics(std::vector<std::string> &topics)
{
    if (Closed())
    {
        return ErrClosedClient;
    }
    std::shared_lock<std::shared_mutex> lk(lock);
    topics.reserve(metadata.size());
    for (auto &kv : metadata)
    {
        topics.push_back(kv.first);
    }
    return 0;
}

int Client::MetadataTopics(std::vector<std::string> &topics)
{
    if (Closed())
    {
        return ErrClosedClient;
    }
    std::shared_lock<std::shared_mutex> lk(lock);
    topics.reserve(metadataTopics.size());
    for (auto &kv : metadataTopics)
    {
        topics.push_back(kv.first);
    }
    return 0;
}

coev::awaitable<int> Client::Partitions(const std::string &topic, std::vector<int32_t> &partitions)
{
    return getPartitions(topic, AllPartitions, partitions);
}

coev::awaitable<int> Client::WritablePartitions(const std::string &topic, std::vector<int32_t> &partitions)
{
    return getPartitions(topic, WritablePartitions_, partitions);
}

coev::awaitable<int> Client::getPartitions(const std::string &topic, int64_t partitionId, std::vector<int32_t> &partitions)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    partitions = cachedPartitions(topic, partitionId);
    if (partitions.empty())
    {
        std::vector<std::string> topics{topic};
        int err = co_await RefreshMetadata(topics);
        if (err != 0)
        {
            co_return err;
        }
        partitions = cachedPartitions(topic, partitionId);
    }
    if (partitions.empty())
    {
        co_return ErrUnknownTopicOrPartition;
    }
    co_return 0;
}

coev::awaitable<int> Client::Replicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &replicas)
{
    std::shared_ptr<PartitionMetadata> m;
    auto err = co_await getReplicas(topic, partitionID, m);
    if (err != 0)
    {
        co_return err;
    }
    replicas = std::move(m->Replicas);
    co_return ErrNoError;
}

coev::awaitable<int> Client::InSyncReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &isr)
{
    std::shared_ptr<PartitionMetadata> m;
    auto err = co_await getReplicas(topic, partitionID, m);
    if (err != 0)
    {
        co_return err;
    }

    isr = std::move(m->Isr);
    co_return ErrNoError;
}

coev::awaitable<int> Client::OfflineReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &offline)
{
    std::shared_ptr<PartitionMetadata> m;
    auto err = co_await getReplicas(topic, partitionID, m);
    if (err != 0)
    {
        co_return err;
    }
    offline = std::move(m->OfflineReplicas);
    co_return ErrNoError;
}

coev::awaitable<int> Client::getReplicas(const std::string &topic, int32_t partitionID, std::shared_ptr<PartitionMetadata> &out)
{

    if (Closed())
    {
        co_return ErrClosedClient;
    }

    out = cachedMetadata(topic, partitionID);
    if (out != nullptr)
    {
        co_return 0;
    }

    std::vector<std::string> topics = {topic};
    int err = co_await RefreshMetadata(topics);
    if (err != 0)
    {
        co_return err;
    }

    out = cachedMetadata(topic, partitionID);
    if (out == nullptr)
    {
        co_return ErrUnknownTopicOrPartition;
    }

    co_return 0;
}

coev::awaitable<int> Client::Leader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader)
{
    int32_t epoch;
    return LeaderAndEpoch(topic, partitionID, leader, epoch);
}

coev::awaitable<int> Client::LeaderAndEpoch(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &leader, int32_t &epoch)
{

    if (Closed())
    {
        co_return ErrClosedClient;
    }
    auto err = co_await cachedLeader(topic, partitionID, leader, epoch);
    if (err != 0)
    {
        std::vector<std::string> topics = {topic};
        err = co_await RefreshMetadata(topics);
        if (err != 0)
        {
            co_return err;
        }
        err = co_await cachedLeader(topic, partitionID, leader, epoch);
    }
    co_return err;
}

coev::awaitable<int> Client::RefreshBrokers(const std::vector<std::string> &addrs)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    std::unique_lock<std::shared_mutex> lk(lock);
    for (auto &kv : brokers)
    {
        kv.second->safeAsyncClose();
    }
    brokers.clear();
    for (auto &b : seedBrokers)
    {
        b->safeAsyncClose();
    }
    seedBrokers.clear();
    for (auto &b : deadSeeds)
    {
        b->safeAsyncClose();
    }
    deadSeeds.clear();
    randomizeSeedBrokers(addrs);
    co_return 0;
}

coev::awaitable<int> Client::RefreshMetadata(const std::vector<std::string> &topics)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    for (auto &t : topics)
    {
        if (t.empty())
        {
            co_return ErrInvalidTopic;
        }
    }
    auto err = co_await metadataRefresh(topics);
    co_return err;
}

coev::awaitable<int> Client::GetOffset(const std::string &topic, int32_t partitionID, int64_t timestamp, int64_t &offset)
{
    if (Closed())
    {
        offset = -1;
        co_return ErrClosedClient;
    }
    int err = co_await getOffset(topic, partitionID, timestamp, offset);
    if (err != 0)
    {
        std::vector<std::string> topics = {topic};
        if ((err = co_await RefreshMetadata(topics)) != 0)
        {
            offset = -1;
            co_return err;
        }
        err = co_await getOffset(topic, partitionID, timestamp, offset);
        co_return err;
    }

    co_return 0;
}

coev::awaitable<int> Client::Controller(std::shared_ptr<Broker> &broker)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    if (!conf_->Version.IsAtLeast(V0_10_0_0))
    {
        co_return ErrUnsupportedVersion;
    }
    broker = cachedController();
    if (broker == nullptr)
    {
        auto err = co_await refreshMetadata();
        if (err != 0)
        {
            co_return err;
        }
        broker = cachedController();
    }
    if (broker == nullptr)
    {
        co_return ErrControllerNotAvailable;
    }
    auto err = co_await broker->Open(conf_);
    co_return err;
}

void Client::deregisterController()
{
    std::unique_lock<std::shared_mutex> lk(lock);
    auto it = brokers.find(controllerID);
    if (it != brokers.end())
    {
        it->second->Close();
        brokers.erase(it);
    }
}

coev::awaitable<int> Client::RefreshController(std::shared_ptr<Broker> &broker)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    deregisterController();
    int err = co_await refreshMetadata();
    if (err != 0)
    {
        co_return err;
    }
    broker = cachedController();
    if (broker == nullptr)
    {
        co_return ErrControllerNotAvailable;
    }
    err = co_await broker->Open(conf_);
    co_return err;
}

coev::awaitable<int> Client::GetCoordinator(const std::string &consumerGroup, std::shared_ptr<Broker> &coordinator)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    coordinator = cachedCoordinator(consumerGroup);
    if (coordinator == nullptr)
    {
        int err = co_await RefreshCoordinator(consumerGroup);
        if (err != 0)
        {
            co_return err;
        }
        coordinator = cachedCoordinator(consumerGroup);
    }
    if (coordinator == nullptr)
    {
        co_return ErrConsumerCoordinatorNotAvailable;
    }
    int err = co_await coordinator->Open(conf_);
    co_return err;
}

coev::awaitable<int> Client::RefreshCoordinator(const std::string &consumerGroup)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    std::shared_ptr<FindCoordinatorResponse> response;
    int err = co_await findCoordinator(consumerGroup, CoordinatorGroup, conf_->Metadata.Retry.Max, response);
    if (err != 0)
    {
        co_return err;
    }
    std::unique_lock<std::shared_mutex> lk(lock);
    registerBroker(response->Coordinator);
    coordinators[consumerGroup] = response->Coordinator->ID();
    co_return 0;
}

coev::awaitable<int> Client::TransactionCoordinator(const std::string &transactionID, std::shared_ptr<Broker> &coordinator)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    coordinator = cachedTransactionCoordinator(transactionID);
    if (coordinator == nullptr)
    {
        int err = co_await RefreshTransactionCoordinator(transactionID);
        if (err != 0)
        {
            co_return err;
        }
        coordinator = cachedTransactionCoordinator(transactionID);
    }
    if (coordinator == nullptr)
    {
        co_return ErrConsumerCoordinatorNotAvailable;
    }
    int err = co_await coordinator->Open(conf_);
    co_return err;
}

coev::awaitable<int> Client::RefreshTransactionCoordinator(const std::string &transactionID)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    std::shared_ptr<FindCoordinatorResponse> response;
    int err = co_await findCoordinator(transactionID, CoordinatorTransaction, conf_->Metadata.Retry.Max, response);
    if (err != 0)
    {
        co_return err;
    }
    std::unique_lock<std::shared_mutex> lk(lock);
    registerBroker(response->Coordinator);
    transactionCoordinators[transactionID] = response->Coordinator->ID();
    co_return 0;
}

void Client::randomizeSeedBrokers(const std::vector<std::string> &addrs)
{
    auto shuffledAddrs = addrs;
    std::random_device rd;
    std::seed_seq seed{rd(), rd(), rd(), rd()};
    std::mt19937 g(seed);
    std::shuffle(shuffledAddrs.begin(), shuffledAddrs.end(), g);

    for (auto &addr : shuffledAddrs)
    {
        seedBrokers.emplace_back(std::make_shared<Broker>(addr));
    }
}

void Client::updateBroker(const std::vector<std::shared_ptr<Broker>> &brokers_list)
{
    std::map<int32_t, std::shared_ptr<Broker>> currentBroker;
    for (auto &broker : brokers_list)
    {
        currentBroker[broker->ID()] = broker;
        auto it = brokers.find(broker->ID());
        if (it == brokers.end())
        {
            brokers[broker->ID()] = broker;
            DebugLogger::Printf("client/brokers registered new broker #%d at %s", broker->ID(), broker->Addr().c_str());
        }
        else if (broker->Addr() != it->second->Addr())
        {
            it->second->safeAsyncClose();
            brokers[broker->ID()] = broker;
            Logger::Printf("client/brokers replaced registered broker #%d with %s", broker->ID(), broker->Addr().c_str());
        }
    }
    for (auto it = brokers.begin(); it != brokers.end();)
    {
        if (currentBroker.find(it->first) == currentBroker.end())
        {
            it->second->safeAsyncClose();
            Logger::Printf("client/broker remove invalid broker #%d with %s", it->second->ID(), it->second->Addr().c_str());
            it = brokers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Client::registerBroker(std::shared_ptr<Broker> broker)
{
    if (brokers.empty())
    {
        Logger::Printf("cannot register broker #%d at %s, client already closed", broker->ID(), broker->Addr().c_str());
        return;
    }
    auto it = brokers.find(broker->ID());
    if (it == brokers.end())
    {
        brokers[broker->ID()] = broker;
        DebugLogger::Printf("client/brokers registered new broker #%d at %s", broker->ID(), broker->Addr().c_str());
    }
    else if (broker->Addr() != it->second->Addr())
    {
        it->second->safeAsyncClose();
        brokers[broker->ID()] = broker;
        Logger::Printf("client/brokers replaced registered broker #%d with %s", broker->ID(), broker->Addr().c_str());
    }
}

void Client::deregisterBroker(std::shared_ptr<Broker> broker)
{
    std::unique_lock<std::shared_mutex> lk(lock);
    auto it = brokers.find(broker->ID());
    if (it != brokers.end())
    {
        Logger::Printf("client/brokers deregistered broker #%d at %s", broker->ID(), broker->Addr().c_str());
        brokers.erase(it);
        return;
    }
    if (!seedBrokers.empty() && broker == seedBrokers[0])
    {
        deadSeeds.push_back(broker);
        seedBrokers.erase(seedBrokers.begin());
    }
}

void Client::resurrectDeadBrokers()
{
    std::unique_lock<std::shared_mutex> lk(lock);
    Logger::Printf("client/brokers resurrecting %zu dead seed brokers", deadSeeds.size());
    seedBrokers.insert(seedBrokers.end(), deadSeeds.begin(), deadSeeds.end());
    deadSeeds.clear();
}

coev::awaitable<int> Client::LeastLoadedBroker(std::shared_ptr<Broker> &leastLoadedBroker)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    size_t pendingRequests = std::numeric_limits<size_t>::max();
    for (auto &kv : brokers)
    {
        if (pendingRequests > kv.second->ResponseSize())
        {
            pendingRequests = kv.second->ResponseSize();
            leastLoadedBroker = kv.second;
        }
    }
    int err = 0;
    if (leastLoadedBroker)
    {
        err = co_await leastLoadedBroker->Open(conf_);
        co_return err;
    }
    if (!seedBrokers.empty())
    {
        err = co_await seedBrokers[0]->Open(conf_);
        leastLoadedBroker = seedBrokers[0];
        co_return err;
    }
    co_return 0;
}

std::shared_ptr<PartitionMetadata> Client::cachedMetadata(const std::string &topic, int32_t partitionID)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto topicIt = metadata.find(topic);
    if (topicIt != metadata.end())
    {
        auto it = topicIt->second.find(partitionID);
        if (it != topicIt->second.end())
        {
            return it->second;
        }
    }
    return nullptr;
}

std::vector<int32_t> Client::cachedPartitions(const std::string &topic, int64_t partitionId)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto it = cachedPartitionsResults.find(topic);
    if (it == cachedPartitionsResults.end())
    {
        return {};
    }
    return it->second[static_cast<size_t>(partitionId)];
}

std::vector<int32_t> Client::setPartitionCache(const std::string &topic, int64_t partitionId)
{
    auto topicIt = metadata.find(topic);
    if (topicIt == metadata.end())
    {
        return {};
    }
    std::vector<int32_t> ret;
    for (auto &kv : topicIt->second)
    {
        if (partitionId == WritablePartitions_ && kv.second->Err == ErrLeaderNotAvailable)
        {
            continue;
        }
        ret.push_back(kv.first);
    }
    sort(ret.begin(), ret.end());
    return ret;
}

coev::awaitable<int> Client::cachedLeader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &broker_, int32_t &leaderEpoch)
{
    broker_ = nullptr;
    leaderEpoch = -1;

    std::shared_lock<std::shared_mutex> lk(lock);
    auto topicIt = metadata.find(topic);
    if (topicIt != metadata.end())
    {
        auto partIt = topicIt->second.find(partitionID);
        if (partIt != topicIt->second.end())
        {
            auto metadata = partIt->second;
            if (metadata->Err == ErrLeaderNotAvailable)
            {
                co_return ErrLeaderNotAvailable;
            }
            auto brokerIt = brokers.find(metadata->Leader);
            if (brokerIt == brokers.end())
            {
                co_return ErrLeaderNotAvailable;
            }
            auto err = co_await brokerIt->second->Open(conf_);
            broker_ = brokerIt->second;
            leaderEpoch = metadata->LeaderEpoch;
            co_return err;
        }
    }
    co_return ErrUnknownTopicOrPartition;
}

coev::awaitable<int> Client::getOffset(const std::string &topic, int32_t partitionID, int64_t timestamp, int64_t &offset)
{
    std::shared_ptr<Broker> broker;
    int err = co_await Leader(topic, partitionID, broker);
    if (err != 0)
    {
        offset = -1;
        co_return err;
    }
    auto request = NewOffsetRequest(conf_->Version);
    request->AddBlock(topic, partitionID, timestamp, 1);
    std::shared_ptr<OffsetResponse> response;
    err = co_await broker->GetAvailableOffsets(request, response);
    if (err != 0)
    {
        co_await broker->Close();
        offset = -1;
        co_return err;
    }
    auto block = response->GetBlock(topic, partitionID);
    if (block == nullptr)
    {
        co_await broker->Close();
        offset = -1;
        co_return ErrIncompleteResponse;
    }
    if (block->Err != ErrNoError)
    {
        offset = -1;
        co_return block->Err;
    }
    if (block->Offsets.size() != 1)
    {
        offset = -1;
        co_return ErrOffsetOutOfRange;
    }
    offset = block->Offsets[0];
    co_return 0;
}

coev::awaitable<int> Client::backgroundMetadataUpdater()
{
    closed.set(false);
    if (conf_->Metadata.RefreshFrequency == std::chrono::milliseconds::zero())
    {
        closed.set(true);
        co_return 0;
    }
    while (true)
    {
        co_await sleep_for(conf_->Metadata.RefreshFrequency);
        if (co_await closer.get())
        {
            break;
        }
        if (int err = co_await refreshMetadata(); err != 0)
        {
            Logger::Println("Client background metadata update:", err);
        }
    }
    closed.set(true);
    co_return 0;
}

coev::awaitable<int> Client::refreshMetadata()
{
    std::vector<std::string> topics;
    if (!conf_->Metadata.Full)
    {
        int err = MetadataTopics(topics);
        if (err != 0)
        {
            co_return err;
        }
        if (topics.empty())
        {
            co_return ErrNoTopicsToUpdateMetadata;
        }
    }
    co_return co_await RefreshMetadata(topics);
}

bool pastDeadline(std::chrono::steady_clock::time_point deadline, std::chrono::milliseconds backoff)
{
    return (deadline.time_since_epoch().count() != 0 && std::chrono::steady_clock::now() + backoff > deadline);
};
coev::awaitable<int> Client::retryRefreshMetadata(const std::vector<std::string> &topics, int attemptsRemaining, std::chrono::steady_clock::time_point deadline, int err)
{

    if (attemptsRemaining > 0)
    {
        auto backoff = ComputeBackoff(attemptsRemaining);
        if (pastDeadline(deadline, backoff))
        {
            Logger::Println("client/metadata skipping last retries as we would go past the metadata timeout");
            co_return err;
        }
        if (backoff.count() > 0)
        {
            co_await sleep_for(backoff);
        }
        int64_t t = updateMetadataMs.load();
        auto lastUpdate = std::chrono::steady_clock::time_point(std::chrono::milliseconds(t));
        if (std::chrono::steady_clock::now() - lastUpdate < backoff)
        {
            co_return err;
        }
        attemptsRemaining--;
        Logger::Printf("client/metadata retrying after %lldms... (%d attempts remaining)\n", backoff.count(), attemptsRemaining);
        co_return co_await tryRefreshMetadata(topics, attemptsRemaining, deadline);
    }
    co_return err;
};
coev::awaitable<int> Client::tryRefreshMetadata(const std::vector<std::string> &topics, int attemptsRemaining, std::chrono::steady_clock::time_point deadline)
{

    int err = ErrNoError;
    std::vector<int> brokerErrors;
    std::shared_ptr<Broker> broker;
    for (err = co_await LeastLoadedBroker(broker); broker != nullptr && !pastDeadline(deadline, std::chrono::milliseconds(0)); err = co_await LeastLoadedBroker(broker))
    {
        bool allowAutoTopicCreation = conf_->Metadata.AllowAutoTopicCreation;
        if (!topics.empty())
        {
            DebugLogger::Printf("client/metadata fetching metadata for %zu topics from broker %s\n", topics.size(), broker->Addr().c_str());
        }
        else
        {
            allowAutoTopicCreation = false;
            DebugLogger::Printf("client/metadata fetching metadata for all topics from broker %s\n", broker->Addr().c_str());
        }
        auto req = NewMetadataRequest(conf_->Version, topics);
        req->AllowAutoTopicCreation = allowAutoTopicCreation;
        updateMetadataMs.store(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        std::shared_ptr<MetadataResponse> response;
        err = co_await broker->GetMetadata(req, response);
        if (err == 0)
        {
            if (response->Brokers.empty())
            {
                Logger::Printf("client/metadata receiving empty brokers from the metadata response when requesting the broker #%d at %s", broker->ID(), broker->Addr().c_str());
                broker->Close();
                deregisterBroker(broker);
                continue;
            }
            bool allKnownMetaData = topics.empty();
            bool shouldRetry;
            shouldRetry = updateMetadata(response, allKnownMetaData);
            if (shouldRetry)
            {
                Logger::Println("client/metadata found some partitions to be leaderless");
                co_return co_await retryRefreshMetadata(topics, attemptsRemaining, deadline, ErrLeaderNotAvailable);
            }
            co_return 0;
        }
        else if (err == ErrEncodeError || err == ErrDecodeError)
        {
            co_return err;
        }
        else if (IsKError(err))
        {
            if (err == ErrSASLAuthenticationFailed)
            {
                Logger::Println("client/metadata failed SASL authentication");
                co_return err;
            }
            if (err == ErrTopicAuthorizationFailed)
            {
                Logger::Println("client is not authorized to access this topic. The topics were: ");
                co_return err;
            }
            Logger::Printf("client/metadata got error from broker %d while fetching metadata: %d\n", broker->ID(), err);
            broker->Close();
            deregisterBroker(broker);
        }
        else
        {
            Logger::Printf("client/metadata got error from broker %d while fetching metadata: %d\n", broker->ID(), err);
            brokerErrors.push_back(err);
            broker->Close();
            deregisterBroker(broker);
        }
    }

    err = co_await LeastLoadedBroker(broker);
    if (err != 0)
    {
        Logger::Printf("client/metadata not fetching metadata from broker as we would go past the metadata timeout\n");
        co_return co_await retryRefreshMetadata(topics, attemptsRemaining, deadline, err);
    }
    Logger::Println("client/metadata no available broker to send metadata request to");
    resurrectDeadBrokers();
    err = co_await retryRefreshMetadata(topics, attemptsRemaining, deadline, err);
    co_return err;
}

bool Client::updateMetadata(std::shared_ptr<MetadataResponse> data, bool allKnownMetaData)
{
    if (Closed())
    {
        return false;
    }
    std::unique_lock<std::shared_mutex> lk(lock);
    updateBroker(data->Brokers);
    controllerID = data->ControllerID;
    if (allKnownMetaData)
    {
        metadata.clear();
        metadataTopics.clear();
        cachedPartitionsResults.clear();
    }
    bool retry = false;
    int err = 0;
    for (auto &topic : data->Topics)
    {
        if (metadataTopics.find(topic->Name) == metadataTopics.end())
        {
            metadataTopics[topic->Name] = true;
        }
        metadata.erase(topic->Name);
        cachedPartitionsResults.erase(topic->Name);
        switch (topic->Err)
        {
        case ErrNoError:
            break;
        case ErrInvalidTopic:
        case ErrTopicAuthorizationFailed:
            err = topic->Err;
            continue;
        case ErrUnknownTopicOrPartition:
            err = topic->Err;
            retry = true;
            continue;
        case ErrLeaderNotAvailable:
            retry = true;
            break;
        default:
            Logger::Printf("Unexpected topic-level metadata error: %s", topic->Err);
            err = topic->Err;
            continue;
        }
        std::map<int32_t, std::shared_ptr<PartitionMetadata>> partitions;
        for (auto &partition : topic->Partitions)
        {
            partitions[partition->ID] = partition;
            if (partition->Err == ErrLeaderNotAvailable)
            {
                retry = true;
            }
        }
        metadata[topic->Name] = partitions;
        std::array<std::vector<int32_t>, MaxPartitionIndex> partitionCache;
        partitionCache[AllPartitions] = setPartitionCache(topic->Name, AllPartitions);
        partitionCache[WritablePartitions_] = setPartitionCache(topic->Name, WritablePartitions_);
        cachedPartitionsResults[topic->Name] = partitionCache;
    }
    return retry;
}

std::shared_ptr<Broker> Client::cachedCoordinator(const std::string &consumerGroup)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto it = coordinators.find(consumerGroup);
    if (it != coordinators.end())
    {
        auto brokerIt = brokers.find(it->second);
        if (brokerIt != brokers.end())
        {
            return brokerIt->second;
        }
    }
    return nullptr;
}

std::shared_ptr<Broker> Client::cachedTransactionCoordinator(const std::string &transactionID)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto it = transactionCoordinators.find(transactionID);
    if (it != transactionCoordinators.end())
    {
        auto brokerIt = brokers.find(it->second);
        if (brokerIt != brokers.end())
        {
            return brokerIt->second;
        }
    }
    return nullptr;
}

std::shared_ptr<Broker> Client::cachedController()
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto it = brokers.find(controllerID);
    if (it != brokers.end())
    {
        return it->second;
    }
    return nullptr;
}

std::chrono::milliseconds Client::ComputeBackoff(int attemptsRemaining)
{
    if (conf_->Metadata.Retry.BackoffFunc)
    {
        int maxRetries = conf_->Metadata.Retry.Max;
        int retries = maxRetries - attemptsRemaining;
        return conf_->Metadata.Retry.BackoffFunc(retries, maxRetries);
    }
    return conf_->Metadata.Retry.Backoff;
}

coev::awaitable<int> Client::findCoordinator(const std::string &coordinatorKey, CoordinatorType coordinatorType, int attemptsRemaining, std::shared_ptr<FindCoordinatorResponse> &response)
{
    auto retry = [&](int err) -> coev::awaitable<int>
    {
        if (attemptsRemaining > 0)
        {
            auto backoff = ComputeBackoff(attemptsRemaining);
            attemptsRemaining--;
            Logger::Printf("client/coordinator retrying after %lldms... (%d attempts remaining)\n", backoff.count(), attemptsRemaining);
            co_await sleep_for(backoff);
            co_return co_await findCoordinator(coordinatorKey, coordinatorType, attemptsRemaining, response);
        }
        co_return err;
    };

    std::vector<int> brokerErrors;
    std::shared_ptr<Broker> broker;
    int err = 0;
    for (err = co_await LeastLoadedBroker(broker); broker != nullptr; err = co_await LeastLoadedBroker(broker))
    {
        DebugLogger::Printf("client/coordinator requesting coordinator for %s from %s\n", coordinatorKey.c_str(), broker->Addr().c_str());
        auto request = std::make_shared<FindCoordinatorRequest>();
        request->CoordinatorKey = coordinatorKey;
        request->CoordinatorType_ = coordinatorType;
        if (conf_->Version.IsAtLeast(V0_11_0_0))
        {
            request->Version = 1;
        }
        if (conf_->Version.IsAtLeast(V2_0_0_0))
        {
            request->Version = 2;
        }
        err = co_await broker->FindCoordinator(request, response);
        if (err != 0)
        {
            Logger::Printf("client/coordinator request to broker %s failed: %d\n", broker->Addr().c_str(), err);
            if (err == ErrNoError)
            {
                co_return err;
            }
            else
            {
                co_await broker->Close();
                brokerErrors.push_back(err);
                deregisterBroker(broker);
                continue;
            }
        }
        if (response->Err == ErrNoError)
        {
            DebugLogger::Printf("client/coordinator coordinator for %s is #%d (%s)\n", coordinatorKey.c_str(), response->Coordinator->ID(), response->Coordinator->Addr().c_str());
            co_return 0;
        }
        else if (response->Err == ErrConsumerCoordinatorNotAvailable)
        {
            Logger::Printf("client/coordinator coordinator for %s is not available\n", coordinatorKey.c_str());
            std::shared_ptr<Broker> leader;
            if (co_await Leader("__consumer_offsets", 0, leader) != 0)
            {
                Logger::Printf("client/coordinator the __consumer_offsets topic is not initialized completely yet. Waiting 2 seconds...\n");
                co_await sleep_for(std::chrono::seconds(2));
            }
            if (coordinatorType == CoordinatorTransaction)
            {
                if (Leader("__transaction_state", 0, leader) != 0)
                {
                    Logger::Printf("client/coordinator the __transaction_state topic is not initialized completely yet. Waiting 2 seconds...\n");
                    co_await sleep_for(std::chrono::seconds(2));
                }
            }
            co_return co_await retry(ErrConsumerCoordinatorNotAvailable);
        }
        else if (response->Err == ErrGroupAuthorizationFailed)
        {
            Logger::Printf("client was not authorized to access group %s while attempting to find coordinator", coordinatorKey.c_str());
            co_return co_await retry(ErrGroupAuthorizationFailed);
        }
        else
        {
            co_return response->Err;
        }
    }
    Logger::Println("client/coordinator no available broker to send consumer metadata request to");
    resurrectDeadBrokers();
    co_return co_await retry(ErrOutOfBrokers);
}

coev::awaitable<int> Client::resolveCanonicalNames(const std::vector<std::string> &addrs, std::vector<std::string> &ips)
{
    // Placeholder: assume DNS resolution is handled externally or not needed in C++ context
    co_return ErrNoError;
}

bool Client::PartitionNotReadable(const std::string &topic, int32_t partition)
{
    std::shared_lock<std::shared_mutex> lk(lock);
    auto topicIt = metadata.find(topic);
    if (topicIt == metadata.end())
    {
        return true;
    }
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
    {
        return true;
    }
    return partIt->second->Leader == -1;
}

coev::awaitable<int> Client::metadataRefresh(const std::vector<std::string> &topics)
{
    std::chrono::steady_clock::time_point deadline{};
    if (conf_->Metadata.Timeout > std::chrono::milliseconds(0))
    {
        deadline = std::chrono::steady_clock::now() + conf_->Metadata.Timeout;
    }
    co_return co_await tryRefreshMetadata(topics, conf_->Metadata.Retry.Max, deadline);
}