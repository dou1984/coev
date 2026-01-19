
#include <random>
#include <algorithm>
#include <chrono>
#include <functional>
#include <coev/coev.h>
#include <coev_dns/parse_dns.h>
#include "utils.h"
#include "client.h"
#include "broker.h"
#include "errors.h"
#include "sleep_for.h"
#include "offset_request.h"
#include "partition_producer.h"

coev::awaitable<int> NewClient(const std::vector<std::string> &addrs, std::shared_ptr<Config> conf, std::shared_ptr<Client> &client_)
{
    LOG_DBG("Initializing new client");
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
            LOG_CORE("NewClient Connecting to Azure Event Hubs: forcing version to V1_0_0_0 for compatibility");
            conf->Version = V1_0_0_0;
        }
    }

    client_ = std::make_shared<Client>(conf);

    client_->m_refresh_metadata.set_refresher(
        [client_, conf](const std::vector<std::string> &topics) -> coev::awaitable<int>
        {
            auto deadline = std::chrono::steady_clock::now();

            if (conf->Metadata.Timeout.count() > 0)
            {
                deadline += conf->Metadata.Timeout;
            }
            co_return co_await client_->TryRefreshMetadata(topics, conf->Metadata.Retry.Max, deadline);
        });

    client_->m_seed_brokers = client_->Brokers();

    if (conf->Net.ResolveCanonicalBootstrapServers)
    {
        std::vector<std::string> resolved_addrs;
        err = co_await client_->ResolveCanonicalNames(addrs, resolved_addrs);
        if (err != 0)
        {
            co_return err;
        }
        client_->RandomizeSeedBrokers(resolved_addrs);
    }
    else
    {
        client_->RandomizeSeedBrokers(addrs);
    }

    if (conf->Metadata.Full)
    {
        err = co_await client_->RefreshMetadata({});
        if (err != 0)
        {
            if (err == ErrLeaderNotAvailable || err == ErrReplicaNotAvailable ||
                err == ErrTopicAuthorizationFailed || err == ErrClusterAuthorizationFailed)
            {
                LOG_CORE("NewClient Error: %d %s", err, KErrorToString(err));
            }
            else
            {
                client_->m_closed.set(true);
                client_->Close();
                co_return err;
            }
        }
    }

    client_->m_task << client_->BackgroundMetadataUpdater();
    LOG_DBG("NewClient Successfully initialized new client");

    co_return ErrNoError;
}

Client::Client(std::shared_ptr<Config> c) : m_conf(c)
{
}
Client::~Client()
{
    LOG_CORE("Client::~Client");
}
std::shared_ptr<Config> Client::GetConfig()
{
    return m_conf;
}

std::vector<std::shared_ptr<Broker>> Client::Brokers()
{
    std::vector<std::shared_ptr<Broker>> _brokers;
    for (auto &kv : m_brokers)
    {
        _brokers.push_back(kv.second);
    }
    return _brokers;
}

coev::awaitable<int> Client::GetBroker(int32_t brokerID, std::shared_ptr<Broker> &broker)
{
    auto it = m_brokers.find(brokerID);
    if (it == m_brokers.end())
    {
        co_return ErrBrokerNotFound;
    }
    broker = it->second;
    auto err = co_await broker->Open(m_conf);
    co_return err;
}

coev::awaitable<int> Client::InitProducerID(InitProducerIDResponse &response)
{
    std::vector<int> brokerErrors;
    std::shared_ptr<Broker> broker;
    int err;
    for (err = co_await LeastLoadedBroker(broker); broker != nullptr; err = co_await LeastLoadedBroker(broker))
    {
        auto request = std::make_shared<InitProducerIDRequest>();
        if (m_conf->Version.IsAtLeast(V2_7_0_0))
        {
            request->m_version = 4;
        }
        else if (m_conf->Version.IsAtLeast(V2_5_0_0))
        {
            request->m_version = 3;
        }
        else if (m_conf->Version.IsAtLeast(V2_4_0_0))
        {
            request->m_version = 2;
        }
        else if (m_conf->Version.IsAtLeast(V2_0_0_0))
        {
            request->m_version = 1;
        }
        ResponsePromise<InitProducerIDResponse> promise;
        err = co_await broker->InitProducerID(*request, promise);
        if (err == 0)
        {
            response = promise.m_response;
            co_return 0;
        }
        else
        {
            LOG_CORE("Client::InitProducerID got error from broker %d when issuing InitProducerID: %d", broker->ID(), err);
            broker->Close();
            brokerErrors.push_back(err);
            DeregisterBroker(broker);
        }
    }
    co_return ErrOutOfBrokers;
}

int Client::Close()
{
    if (Closed())
    {
        LOG_CORE("Client::Close called on already closed client");
        return ErrClosedClient;
    }
    m_closer.set(true);

    LOG_CORE("Client::Close Closing Client");
    for (auto &kv : m_brokers)
    {
        kv.second->SafeAsyncClose();
    }
    for (auto &b : m_seed_brokers)
    {
        b->SafeAsyncClose();
    }
    // 不要在非协程上下文调用m_closed.get()，这会导致段错误
    // m_closed.get();
    m_brokers.clear();
    m_dead_seeds.clear();
    m_metadata.clear();
    m_metadata_topics.clear();
    return 0;
}

bool Client::Closed()
{
    return m_brokers.empty() && m_seed_brokers.empty();
}

int Client::Topics(std::vector<std::string> &topics)
{
    if (Closed())
    {
        return ErrClosedClient;
    }
    topics.reserve(m_metadata.size());
    for (auto &kv : m_metadata)
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
    topics.reserve(m_metadata_topics.size());
    for (auto &kv : m_metadata_topics)
    {
        topics.push_back(kv.first);
    }
    return 0;
}

coev::awaitable<int> Client::Partitions(const std::string &topic, std::vector<int32_t> &partitions)
{
    return _GetPartitions(topic, AllPartitions, partitions);
}

coev::awaitable<int> Client::WritablePartitions(const std::string &topic, std::vector<int32_t> &partitions)
{
    return _GetPartitions(topic, WritablePartitions_, partitions);
}

coev::awaitable<int> Client::_GetPartitions(const std::string &topic, int64_t partitionId, std::vector<int32_t> &partitions)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    partitions = _CachedPartitions(topic, partitionId);
    if (partitions.empty())
    {
        std::vector<std::string> topics{topic};
        int err = co_await RefreshMetadata(topics);
        if (err != 0)
        {
            co_return err;
        }
        partitions = _CachedPartitions(topic, partitionId);
    }
    if (partitions.empty())
    {
        co_return ErrUnknownTopicOrPartition;
    }
    co_return 0;
}

coev::awaitable<int> Client::Replicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &replicas)
{
    PartitionMetadata m;
    auto err = co_await _GetReplicas(topic, partitionID, m);
    if (err != 0)
    {
        co_return err;
    }
    replicas = std::move(m.m_replicas);
    co_return ErrNoError;
}

coev::awaitable<int> Client::InSyncReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &isr)
{
    PartitionMetadata m;
    auto err = co_await _GetReplicas(topic, partitionID, m);
    if (err != 0)
    {
        co_return err;
    }

    isr = std::move(m.m_isr);
    co_return ErrNoError;
}

coev::awaitable<int> Client::OfflineReplicas(const std::string &topic, int32_t partitionID, std::vector<int32_t> &offline)
{
    PartitionMetadata m;
    auto err = co_await _GetReplicas(topic, partitionID, m);
    if (err != 0)
    {
        co_return err;
    }

    offline = std::move(m.m_offline_replicas);
    co_return ErrNoError;
}

coev::awaitable<int> Client::_GetReplicas(const std::string &topic, int32_t partitionID, PartitionMetadata &out)
{

    if (Closed())
    {
        co_return ErrClosedClient;
    }

    auto cached = _CachedMetadata(topic, partitionID);
    if (cached != nullptr)
    {
        out = *cached;
        co_return 0;
    }

    std::vector<std::string> topics = {topic};
    int err = co_await RefreshMetadata(topics);
    if (err != 0)
    {
        co_return err;
    }

    cached = _CachedMetadata(topic, partitionID);
    if (cached == nullptr)
    {
        co_return ErrUnknownTopicOrPartition;
    }

    out = *cached;
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
    auto err = co_await _CachedLeader(topic, partitionID, leader, epoch);
    if (err != 0)
    {
        std::vector<std::string> topics = {topic};
        err = co_await RefreshMetadata(topics);
        if (err != 0)
        {
            co_return err;
        }
        err = co_await _CachedLeader(topic, partitionID, leader, epoch);
    }
    co_return err;
}

coev::awaitable<int> Client::RefreshBrokers(const std::vector<std::string> &addrs)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    for (auto &kv : m_brokers)
    {
        kv.second->SafeAsyncClose();
    }
    m_brokers.clear();
    for (auto &b : m_seed_brokers)
    {
        b->SafeAsyncClose();
    }
    m_seed_brokers.clear();
    for (auto &b : m_dead_seeds)
    {
        b->SafeAsyncClose();
    }
    m_dead_seeds.clear();
    RandomizeSeedBrokers(addrs);
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
    auto _topics = m_refresh_metadata.add_topics(topics);
    co_return co_await m_refresh_metadata.refresh(topics);
}

coev::awaitable<int> Client::GetOffset(const std::string &topic, int32_t partitionID, int64_t timestamp, int64_t &offset)
{
    if (Closed())
    {
        offset = -1;
        co_return ErrClosedClient;
    }
    int err = co_await _GetOffset(topic, partitionID, timestamp, offset);
    if (err != 0)
    {
        std::vector<std::string> topics = {topic};
        if ((err = co_await RefreshMetadata(topics)) != 0)
        {
            offset = -1;
            co_return err;
        }
        err = co_await _GetOffset(topic, partitionID, timestamp, offset);
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
    if (!m_conf->Version.IsAtLeast(V0_10_0_0))
    {
        co_return ErrUnsupportedVersion;
    }
    broker = _CachedController();
    if (broker == nullptr)
    {
        auto err = co_await RefreshMetadata();
        if (err != 0)
        {
            co_return err;
        }
        broker = _CachedController();
    }
    if (broker == nullptr)
    {
        co_return ErrControllerNotAvailable;
    }
    auto err = co_await broker->Open(m_conf);
    co_return err;
}

void Client::DeregisterController()
{
    auto it = m_brokers.find(m_controller_id);
    if (it != m_brokers.end())
    {
        it->second->Close();
        m_brokers.erase(it);
    }
}

coev::awaitable<int> Client::RefreshController(std::shared_ptr<Broker> &broker)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    DeregisterController();
    int err = co_await RefreshMetadata();
    if (err != 0)
    {
        co_return err;
    }
    broker = _CachedController();
    if (broker == nullptr)
    {
        co_return ErrControllerNotAvailable;
    }
    err = co_await broker->Open(m_conf);
    co_return err;
}

coev::awaitable<int> Client::GetCoordinator(const std::string &consumerGroup, std::shared_ptr<Broker> &coordinator)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    coordinator = _CachedCoordinator(consumerGroup);
    if (coordinator == nullptr)
    {
        int err = co_await RefreshCoordinator(consumerGroup);
        if (err != 0)
        {
            co_return err;
        }
        coordinator = _CachedCoordinator(consumerGroup);
    }
    if (coordinator == nullptr)
    {
        co_return ErrConsumerCoordinatorNotAvailable;
    }
    int err = co_await coordinator->Open(m_conf);
    co_return err;
}

coev::awaitable<int> Client::RefreshCoordinator(const std::string &consumerGroup)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    FindCoordinatorResponse response;
    int err = co_await FindCoordinator(consumerGroup, CoordinatorGroup, m_conf->Metadata.Retry.Max, response);
    if (err != 0)
    {
        co_return err;
    }
    RegisterBroker(response.m_coordinator);
    m_coordinators[consumerGroup] = response.m_coordinator->ID();
    co_return 0;
}

coev::awaitable<int> Client::TransactionCoordinator(const std::string &transactionID, std::shared_ptr<Broker> &coordinator)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    coordinator = _CachedTransactionCoordinator(transactionID);
    if (coordinator == nullptr)
    {
        int err = co_await RefreshTransactionCoordinator(transactionID);
        if (err != 0)
        {
            co_return err;
        }
        coordinator = _CachedTransactionCoordinator(transactionID);
    }
    if (coordinator == nullptr)
    {
        co_return ErrConsumerCoordinatorNotAvailable;
    }
    int err = co_await coordinator->Open(m_conf);
    co_return err;
}

coev::awaitable<int> Client::RefreshTransactionCoordinator(const std::string &transactionID)
{
    if (Closed())
    {
        co_return ErrClosedClient;
    }
    FindCoordinatorResponse response;
    int err = co_await FindCoordinator(transactionID, CoordinatorTransaction, m_conf->Metadata.Retry.Max, response);
    if (err != 0)
    {
        co_return err;
    }
    RegisterBroker(response.m_coordinator);
    m_transaction_coordinators[transactionID] = response.m_coordinator->ID();
    co_return 0;
}

void Client::RandomizeSeedBrokers(const std::vector<std::string> &addrs)
{
    // Clear existing seed brokers before adding new ones
    m_seed_brokers.clear();

    auto shuffledAddrs = addrs;
    std::random_device rd;
    std::seed_seq seed{rd(), rd(), rd(), rd()};
    std::mt19937 g(seed);
    std::shuffle(shuffledAddrs.begin(), shuffledAddrs.end(), g);

    for (auto &addr : shuffledAddrs)
    {
        m_seed_brokers.emplace_back(std::make_shared<Broker>(addr));
    }
}

void Client::UpdateBroker(const std::vector<std::shared_ptr<Broker>> &_brokers_list)
{
    std::map<int32_t, std::shared_ptr<Broker>> currentBroker;
    for (auto &broker : _brokers_list)
    {
        currentBroker[broker->ID()] = broker;
        auto it = m_brokers.find(broker->ID());
        if (it == m_brokers.end())
        {
            m_brokers[broker->ID()] = broker;
            LOG_DBG("Client::UpdateBrokers registered new broker: #%d at %s", broker->ID(), broker->Addr().c_str());
        }
        else if (broker->Addr() != it->second->Addr())
        {
            it->second->SafeAsyncClose();
            m_brokers[broker->ID()] = broker;
            LOG_CORE("Client::UpdateBrokers replaced registered broker: #%d with %s", broker->ID(), broker->Addr().c_str());
        }
    }
    for (auto it = m_brokers.begin(); it != m_brokers.end();)
    {
        if (currentBroker.find(it->first) == currentBroker.end())
        {
            it->second->SafeAsyncClose();
            LOG_CORE("Client::UpdateBrokers remove invalid broker: #%d with %s", it->second->ID(), it->second->Addr().c_str());
            it = m_brokers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void Client::RegisterBroker(std::shared_ptr<Broker> broker)
{

    // 直接检查关闭条件，避免死锁
    if (m_brokers.empty() && m_seed_brokers.empty())
    {
        LOG_CORE("Client::RegisterBroker cannot register broker: #%d at %s, client already closed", broker->ID(), broker->Addr().c_str());
        return;
    }
    auto it = m_brokers.find(broker->ID());
    if (it == m_brokers.end())
    {
        m_brokers[broker->ID()] = broker;
        LOG_DBG("Client::RegisterBroker registered new broker: #%d at %s", broker->ID(), broker->Addr().c_str());
    }
    else if (broker->Addr() != it->second->Addr())
    {
        it->second->SafeAsyncClose();
        m_brokers[broker->ID()] = broker;
        LOG_CORE("Client::RegisterBroker replaced registered broker: #%d with %s", broker->ID(), broker->Addr().c_str());
    }
}

void Client::DeregisterBroker(std::shared_ptr<Broker> broker)
{

    auto it = m_brokers.find(broker->ID());
    if (it != m_brokers.end())
    {
        LOG_CORE("Client::DeregisterBroker deregistered broker: #%d at %s", broker->ID(), broker->Addr().c_str());
        m_brokers.erase(it);
        return;
    }
    if (!m_seed_brokers.empty() && broker == m_seed_brokers[0])
    {
        m_dead_seeds.push_back(broker);
        m_seed_brokers.erase(m_seed_brokers.begin());
    }
}

void Client::ResurrectDeadBrokers()
{

    LOG_CORE("Client::ResurrectDeadBrokers resurrecting dead seed brokers: %zu", m_dead_seeds.size());
    m_seed_brokers.insert(m_seed_brokers.end(), m_dead_seeds.begin(), m_dead_seeds.end());
    m_dead_seeds.clear();
}

coev::awaitable<int> Client::LeastLoadedBroker(std::shared_ptr<Broker> &leastLoadedBroker)
{

    for (auto &kv : m_brokers)
    {
        leastLoadedBroker = kv.second;
    }
    int err = 0;
    if (leastLoadedBroker)
    {
        err = co_await leastLoadedBroker->Open(m_conf);
        if (err != 0)
        {
            co_return err;
        }
        co_return err;
    }
    if (!m_seed_brokers.empty())
    {
        std::shared_ptr<Broker> seedBroker = m_seed_brokers[0];
        err = co_await seedBroker->Open(m_conf);
        if (err != 0)
        {
            if (!m_seed_brokers.empty() && m_seed_brokers[0] == seedBroker)
            {
                m_seed_brokers.erase(m_seed_brokers.begin());
                m_dead_seeds.push_back(seedBroker);
            }
            co_return err;
        }

        if (!m_seed_brokers.empty() && m_seed_brokers[0] == seedBroker)
        {
            leastLoadedBroker = seedBroker;
        }
        else
        {
            err = ErrBrokerNotAvailable;
        }
        co_return err;
    }
    co_return ErrBrokerNotAvailable;
}

std::shared_ptr<PartitionMetadata> Client::_CachedMetadata(const std::string &topic, int32_t partitionID)
{

    auto topicIt = m_metadata.find(topic);
    if (topicIt != m_metadata.end())
    {
        auto it = topicIt->second.find(partitionID);
        if (it != topicIt->second.end())
        {
            return it->second;
        }
    }
    return nullptr;
}

std::vector<int32_t> Client::_CachedPartitions(const std::string &topic, int64_t partitionId)
{

    auto it = m_cached_partitions_results.find(topic);
    if (it == m_cached_partitions_results.end())
    {
        return {};
    }
    return it->second[static_cast<size_t>(partitionId)];
}

std::vector<int32_t> Client::_SetPartitionCache(const std::string &topic, int64_t partitionId)
{
    auto topicIt = m_metadata.find(topic);
    if (topicIt == m_metadata.end())
    {
        return {};
    }
    std::vector<int32_t> ret;
    for (auto &kv : topicIt->second)
    {
        if (partitionId == WritablePartitions_ && kv.second->m_err == ErrLeaderNotAvailable)
        {
            continue;
        }
        ret.push_back(kv.first);
    }
    sort(ret.begin(), ret.end());
    return ret;
}

coev::awaitable<int> Client::_CachedLeader(const std::string &topic, int32_t partitionID, std::shared_ptr<Broker> &broker_, int32_t &leaderEpoch)
{
    broker_ = nullptr;
    leaderEpoch = -1;

    auto topicIt = m_metadata.find(topic);
    if (topicIt != m_metadata.end())
    {
        auto partIt = topicIt->second.find(partitionID);
        if (partIt != topicIt->second.end())
        {
            auto m_metadata = partIt->second;
            if (m_metadata->m_err == ErrLeaderNotAvailable)
            {
                co_return ErrLeaderNotAvailable;
            }
            auto brokerIt = m_brokers.find(m_metadata->m_leader);
            if (brokerIt == m_brokers.end())
            {
                co_return ErrLeaderNotAvailable;
            }
            auto err = co_await brokerIt->second->Open(m_conf);
            broker_ = brokerIt->second;
            leaderEpoch = m_metadata->m_leader_epoch;
            co_return err;
        }
    }
    co_return ErrUnknownTopicOrPartition;
}

coev::awaitable<int> Client::_GetOffset(const std::string &topic, int32_t partitionID, int64_t timestamp, int64_t &offset)
{
    std::shared_ptr<Broker> broker;
    int err = co_await Leader(topic, partitionID, broker);
    if (err != 0)
    {
        offset = -1;
        co_return err;
    }
    auto request = NewOffsetRequest(m_conf->Version);
    request->AddBlock(topic, partitionID, timestamp, 1);
    ResponsePromise<OffsetResponse> promise;
    err = co_await broker->GetAvailableOffsets(*request, promise);
    if (err != 0)
    {
        co_await broker->Close();
        offset = -1;
        co_return err;
    }
    auto block = promise.m_response.GetBlock(topic, partitionID);
    if (block == nullptr)
    {
        co_await broker->Close();
        offset = -1;
        co_return ErrIncompleteResponse;
    }
    if (block->m_err != ErrNoError)
    {
        offset = -1;
        co_return block->m_err;
    }
    if (block->m_offsets.size() != 1)
    {
        offset = -1;
        co_return ErrOffsetOutOfRange;
    }
    offset = block->m_offsets[0];
    co_return 0;
}

coev::awaitable<int> Client::BackgroundMetadataUpdater()
{
    m_closed.set(false);
    if (m_conf->Metadata.RefreshFrequency == std::chrono::milliseconds::zero())
    {
        m_closed.set(true);
        co_return 0;
    }
    co_await wait_for_any(
        [this]() -> coev::awaitable<void>
        { co_await m_closer.get(); }(),
        [this]() -> coev::awaitable<void>
        {
            while (true)
            {
                co_await sleep_for(m_conf->Metadata.RefreshFrequency);
                if (int err = co_await RefreshMetadata(); err != 0)
                {
                    LOG_CORE("background metadata update: %d", err);
                }
            }
        }());
    m_closed.set(true);
    co_return 0;
}

coev::awaitable<int> Client::RefreshMetadata()
{
    std::vector<std::string> topics;
    if (!m_conf->Metadata.Full)
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
    int err = co_await RefreshMetadata(topics);
    co_return err;
}

bool pastDeadline(std::chrono::steady_clock::time_point deadline, std::chrono::milliseconds backoff)
{
    return (deadline.time_since_epoch().count() != 0 && std::chrono::steady_clock::now() + backoff > deadline);
};
coev::awaitable<int> Client::RetryRefreshMetadata(const std::vector<std::string> &topics, int attemptsRemaining, std::chrono::steady_clock::time_point deadline, int err)
{

    if (attemptsRemaining > 0)
    {
        auto backoff = ComputeBackoff(attemptsRemaining);
        if (pastDeadline(deadline, backoff))
        {
            LOG_CORE("skipping last retries as we would go past the metadata timeout");
            co_return err;
        }
        if (backoff.count() > 0)
        {
            co_await sleep_for(backoff);
        }
        int64_t t = m_update_metadata_ms.load();
        auto lastUpdate = std::chrono::steady_clock::time_point(std::chrono::milliseconds(t));
        if (std::chrono::steady_clock::now() - lastUpdate < backoff)
        {
            co_return err;
        }
        --attemptsRemaining;
        LOG_CORE("retrying after %ldms... (%d attempts remaining)", backoff.count(), attemptsRemaining);
        co_return co_await TryRefreshMetadata(topics, attemptsRemaining, deadline);
    }
    co_return err;
};
coev::awaitable<int> Client::TryRefreshMetadata(const std::vector<std::string> &topics, int attemptsRemaining, std::chrono::steady_clock::time_point deadline)
{

    defer(LOG_CORE("metadata refesh finished"));
    int err = ErrNoError;
    std::vector<int> brokerErrors;
    std::shared_ptr<Broker> broker;
    co_await LeastLoadedBroker(broker);
    for (; broker && !pastDeadline(deadline, std::chrono::milliseconds(0)); co_await LeastLoadedBroker(broker))
    {
        bool allowAutoTopicCreation = m_conf->Metadata.AllowAutoTopicCreation;
        if (!topics.empty())
        {
            LOG_CORE("fetching metadata for %zu topics from broker %s", topics.size(), broker->Addr().c_str());
        }
        else
        {
            allowAutoTopicCreation = false;
            LOG_CORE("fetching metadata for all topics from broker %s", broker->Addr().c_str());
        }
        auto request = NewMetadataRequest(m_conf->Version, topics);
        request->m_allow_auto_topic_creation = allowAutoTopicCreation;
        m_update_metadata_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        ResponsePromise<MetadataResponse> promise;
        err = co_await broker->GetMetadata(*request, promise);
        if (err == 0)
        {
            auto &response = promise.m_response;
            if (response.m_brokers.empty())
            {
                LOG_CORE("receiving empty brokers from the metadata response when requesting the broker #%d at %s", broker->ID(), broker->Addr().c_str());
                broker->Close();
                DeregisterBroker(broker);
                continue;
            }
            bool allKnownMetaData = topics.empty();
            bool shouldRetry;

            auto responsePtr = std::make_shared<MetadataResponse>(response);
            shouldRetry = UpdateMetadata(responsePtr, allKnownMetaData);
            if (shouldRetry)
            {
                LOG_CORE("found some partitions to be leaderless");
                err = co_await RetryRefreshMetadata(topics, attemptsRemaining, deadline, ErrLeaderNotAvailable);
                co_return err;
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
                LOG_CORE("failed SASL authentication");
                co_return err;
            }
            if (err == ErrTopicAuthorizationFailed)
            {
                LOG_CORE("client is not authorized to access this topic. The topics were: ");
                co_return err;
            }
            LOG_CORE("got error from broker %d while fetching metadata: %d", broker->ID(), err);
            broker->Close();
            DeregisterBroker(broker);
        }
        else
        {
            LOG_CORE("got error from broker %d while fetching metadata: %d", broker->ID(), err);
            brokerErrors.push_back(err);
            broker->Close();
            DeregisterBroker(broker);
        }
    }

    LOG_CORE("no available broker to send metadata request to");
    ResurrectDeadBrokers();
    co_return co_await RetryRefreshMetadata(topics, attemptsRemaining, deadline, err);
}

bool Client::UpdateMetadata(std::shared_ptr<MetadataResponse> data, bool allKnownMetaData)
{
    if (Closed())
    {
        return false;
    }

    UpdateBroker(data->m_brokers);
    m_controller_id = data->m_controller_id;
    if (allKnownMetaData)
    {
        m_metadata.clear();
        m_metadata_topics.clear();
        m_cached_partitions_results.clear();
    }
    bool retry = false;
    int err = 0;
    for (auto &topic : data->m_topics)
    {
        if (m_metadata_topics.find(topic->Name) == m_metadata_topics.end())
        {
            m_metadata_topics[topic->Name] = true;
        }
        m_metadata.erase(topic->Name);
        m_cached_partitions_results.erase(topic->Name);
        switch (topic->m_err)
        {
        case ErrNoError:
            break;
        case ErrInvalidTopic:
        case ErrTopicAuthorizationFailed:
            err = topic->m_err;
            continue;
        case ErrUnknownTopicOrPartition:
            err = topic->m_err;
            retry = true;
            continue;
        case ErrLeaderNotAvailable:
            retry = true;
            break;
        default:
            LOG_CORE("Unexpected topic-level metadata error: %s", KErrorToString(topic->m_err));
            err = topic->m_err;
            continue;
        }
        std::map<int32_t, std::shared_ptr<PartitionMetadata>> partitions;
        for (auto &partition : topic->Partitions)
        {
            partitions[partition->m_id] = partition;
            if (partition->m_err == ErrLeaderNotAvailable)
            {
                retry = true;
            }
        }
        m_metadata[topic->Name] = partitions;
        std::array<std::vector<int32_t>, MaxPartitionIndex> partitionCache;
        partitionCache[AllPartitions] = _SetPartitionCache(topic->Name, AllPartitions);
        partitionCache[WritablePartitions_] = _SetPartitionCache(topic->Name, WritablePartitions_);
        m_cached_partitions_results[topic->Name] = partitionCache;
    }
    return retry;
}

std::shared_ptr<Broker> Client::_CachedCoordinator(const std::string &consumerGroup)
{

    auto it = m_coordinators.find(consumerGroup);
    if (it != m_coordinators.end())
    {
        auto brokerIt = m_brokers.find(it->second);
        if (brokerIt != m_brokers.end())
        {
            return brokerIt->second;
        }
    }
    return nullptr;
}

std::shared_ptr<Broker> Client::_CachedTransactionCoordinator(const std::string &transactionID)
{

    auto it = m_transaction_coordinators.find(transactionID);
    if (it != m_transaction_coordinators.end())
    {
        auto brokerIt = m_brokers.find(it->second);
        if (brokerIt != m_brokers.end())
        {
            return brokerIt->second;
        }
    }
    return nullptr;
}

std::shared_ptr<Broker> Client::_CachedController()
{

    auto it = m_brokers.find(m_controller_id);
    if (it != m_brokers.end())
    {
        return it->second;
    }
    return nullptr;
}

std::chrono::milliseconds Client::ComputeBackoff(int attemptsRemaining)
{
    if (m_conf->Metadata.Retry.BackoffFunc)
    {
        int maxRetries = m_conf->Metadata.Retry.Max;
        int retries = maxRetries - attemptsRemaining;
        return m_conf->Metadata.Retry.BackoffFunc(retries, maxRetries);
    }
    return m_conf->Metadata.Retry.Backoff;
}

coev::awaitable<int> Client::FindCoordinator(const std::string &coordinatorKey, CoordinatorType coordinatorType, int attemptsRemaining, FindCoordinatorResponse &response)
{
    auto retry = [&](int err) -> coev::awaitable<int>
    {
        if (attemptsRemaining > 0)
        {
            auto backoff = ComputeBackoff(attemptsRemaining);
            attemptsRemaining--;
            LOG_CORE("retrying after %ldms %d attempts remaining", backoff.count(), attemptsRemaining);
            co_await sleep_for(backoff);
            co_return co_await FindCoordinator(coordinatorKey, coordinatorType, attemptsRemaining, response);
        }
        co_return err;
    };

    std::vector<int> brokerErrors;
    std::shared_ptr<Broker> broker;
    int err = 0;
    for (err = co_await LeastLoadedBroker(broker); broker != nullptr; err = co_await LeastLoadedBroker(broker))
    {
        LOG_DBG("requesting coordinator for %s from %s", coordinatorKey.c_str(), broker->Addr().c_str());
        auto request = std::make_shared<FindCoordinatorRequest>();
        request->m_coordinator_key = coordinatorKey;
        request->m_coordinator_type = coordinatorType;
        if (m_conf->Version.IsAtLeast(V0_11_0_0))
        {
            request->m_version = 1;
        }
        if (m_conf->Version.IsAtLeast(V2_0_0_0))
        {
            request->m_version = 2;
        }
        ResponsePromise<FindCoordinatorResponse> promise;
        err = co_await broker->FindCoordinator(*request, promise);
        if (err != 0)
        {
            LOG_CORE("request to broker %s failed: %d", broker->Addr().c_str(), err);
            if (err == ErrNoError)
            {
                co_return err;
            }
            else
            {
                co_await broker->Close();
                brokerErrors.push_back(err);
                DeregisterBroker(broker);
                continue;
            }
        }
        response = promise.m_response;
        if (response.m_err == ErrNoError)
        {
            LOG_DBG("coordinator for %s is #%d (%s)", coordinatorKey.c_str(), response.m_coordinator->ID(), response.m_coordinator->Addr().c_str());
            co_return 0;
        }
        else if (response.m_err == ErrConsumerCoordinatorNotAvailable)
        {
            LOG_CORE("coordinator for %s is not available", coordinatorKey.c_str());
            std::shared_ptr<Broker> leader;
            if (co_await Leader("__consumer_offsets", 0, leader) != 0)
            {
                LOG_CORE("the __consumer_offsets topic is not initialized completely yet. Waiting 2 seconds...");
                co_await sleep_for(std::chrono::seconds(2));
            }
            if (coordinatorType == CoordinatorTransaction)
            {
                if (Leader("__transaction_state", 0, leader) != 0)
                {
                    LOG_CORE("the __transaction_state topic is not initialized completely yet. Waiting 2 seconds...");
                    co_await sleep_for(std::chrono::seconds(2));
                }
            }
            co_return co_await retry(ErrConsumerCoordinatorNotAvailable);
        }
        else if (response.m_err == ErrGroupAuthorizationFailed)
        {
            LOG_CORE("client was not authorized to access group %s while attempting to find coordinator", coordinatorKey.c_str());
            co_return co_await retry(ErrGroupAuthorizationFailed);
        }
        else
        {
            co_return response.m_err;
        }
    }
    LOG_CORE("no available broker to send consumer metadata request to");
    ResurrectDeadBrokers();
    err = co_await retry(ErrOutOfBrokers);
    co_return err;
}

coev::awaitable<int> Client::ResolveCanonicalNames(const std::vector<std::string> &addrs, std::vector<std::string> &ips)
{
    ips.resize(addrs.size());
    co_task task;
    for (auto i = 0; i < addrs.size(); ++i)
    {
        auto &addr = addrs[i];
        std::regex re("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(:\\d{1,5})?$");
        if (std::regex_match(addr, re))
        {
            ips[i] = addr;
            continue;
        }
        task << [&]() -> coev::awaitable<void>
        {
            auto err = co_await coev::parse_dns(addr, ips[i]);
            if (err != 0)
            {
                LOG_CORE("failed to resolve canonical name for %s: %d", addr.c_str(), err);
                co_return;
            }
        }();
    }
    co_await task.wait_all();
    co_return ErrNoError;
}

bool Client::PartitionNotReadable(const std::string &topic, int32_t partition)
{

    auto topicIt = m_metadata.find(topic);
    if (topicIt == m_metadata.end())
    {
        return true;
    }
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
    {
        return true;
    }
    return partIt->second->m_leader == -1;
}

coev::awaitable<int> Client::MetadataRefresh(const std::vector<std::string> &topics)
{
    std::chrono::steady_clock::time_point deadline{};
    if (m_conf->Metadata.Timeout > std::chrono::milliseconds(0))
    {
        deadline = std::chrono::steady_clock::now() + m_conf->Metadata.Timeout;
    }
    co_return co_await TryRefreshMetadata(topics, m_conf->Metadata.Retry.Max, deadline);
}
