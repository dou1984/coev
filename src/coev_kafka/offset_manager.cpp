#include <chrono>
#include <queue>
#include <shared_mutex>
#include <atomic>
#include "version.h"
#include "offset_manager.h"
#include "sleep_for.h"
#include "partition_offset_manager.h"

const int32_t GroupGenerationUndefined = -1;
const int64_t ReceiveTime = -1;

coev::awaitable<int> OffsetManager::NewPartitionOffsetManager(
    const std::string &topic, int32_t partition, std::shared_ptr<PartitionOffsetManager> &pom)
{

    int64_t offset;
    int32_t leaderEpoch;
    std::string metadata;
    auto err = co_await fetchInitialOffset(topic, partition, m_conf->Metadata.Retry.Max, offset, leaderEpoch, metadata);
    if (err != 0)
    {
        co_return err;
    }

    pom = std::make_shared<PartitionOffsetManager>();
    pom->m_parent = shared_from_this();
    pom->m_topic = topic;
    pom->m_partition = partition;
    pom->m_leaderEpoch = leaderEpoch;
    pom->m_offset = offset;
    pom->m_metadata = metadata;
    co_return ErrNoError;
}

OffsetManager::OffsetManager(
    std::shared_ptr<Client> client,
    std::shared_ptr<Config> conf,
    const std::string &group,
    std::function<void()> sessionCanceler,
    const std::string &memberID,
    int32_t generation)
    : m_client(client), m_conf(conf), m_group(group), m_session_canceler(sessionCanceler), m_member_id(memberID), m_generation(generation), m_closing(false) {}

std::shared_ptr<PartitionOffsetManager> OffsetManager::ManagePartition(const std::string &topic, int32_t partition)
{
    auto pom = std::make_shared<PartitionOffsetManager>(shared_from_this(), topic, partition, 0, m_conf->ChannelBufferSize, 0, "");

    {

        if (m_poms.count(topic) && m_poms.at(topic).count(partition))
        {
            throw std::runtime_error("That topic/partition is already being managed");
        }
    }

    {
        if (!m_poms.count(topic))
        {
            m_poms[topic] = {};
        }
        m_poms[topic][partition] = std::dynamic_pointer_cast<PartitionOffsetManager>(pom);
    }

    return pom;
}

coev::awaitable<int> OffsetManager::Close()
{

    if (m_closing)
        co_return ErrNoError;
    m_closing = true;
    if (m_conf->Consumer.Offsets.AutoCommit.Enable)
    {
        m_closed.set(true);
    }

    asyncClosePOMs();

    if (m_conf->Consumer.Offsets.AutoCommit.Enable)
    {
        for (int attempt = 0; attempt <= m_conf->Consumer.Offsets.Retry.Max; ++attempt)
        {
            co_await flushToBroker();
            if (releasePOMs(false) == 0)
                break;
        }
    }

    releasePOMs(true);

    std::unique_lock<std::shared_mutex> lock(m_broker_lock);
    m_broker = nullptr;
    co_return ErrNoError;
}

std::chrono::milliseconds OffsetManager::ComputeBackoff(int retries)
{
    if (m_conf->Metadata.Retry.BackoffFunc)
    {
        return m_conf->Metadata.Retry.BackoffFunc(retries, m_conf->Metadata.Retry.Max);
    }
    else
    {
        return m_conf->Metadata.Retry.Backoff;
    }
}

coev::awaitable<int> OffsetManager::fetchInitialOffset(const std::string &topic, int32_t partition, int retries, int64_t &offset, int32_t &leaderEpoch, std::string &metadata)
{
    std::shared_ptr<Broker> broker;
    auto err = co_await coordinator(broker);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err;
        }
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    }

    std::map<std::string, std::vector<int32_t>> partitions;
    partitions[topic] = {partition};
    auto req = OffsetFetchRequest::NewOffsetFetchRequest(m_conf->Version, m_group, partitions);
    ResponsePromise<OffsetFetchResponse> resp;
    err = co_await broker->FetchOffset(*req, resp);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err; // 简化表示 FetchOffset 失败
        }
        releaseCoordinator(broker);
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    }

    auto block = resp.m_response.GetBlock(topic, partition);
    if (!block)
    {
        co_return ErrIncompleteResponse;
    }

    switch (block->m_err)
    {
    case ErrNoError:
        offset = block->m_offset;
        leaderEpoch = block->m_leader_epoch;
        metadata = block->m_metadata;
        co_return 0;
    case ErrNotCoordinatorForConsumer:
        if (retries <= 0)
        {
            co_return block->m_err;
        }
        releaseCoordinator(broker);
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return block->m_err;
        }
        {
            auto backoff = ComputeBackoff(retries);

            m_closing = true;
            if (sleep_for(std::chrono::milliseconds(backoff)))
            {
                m_closed.set(true);
            }
        }
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    default:
        co_return block->m_err;
    }
}
coev::awaitable<int> OffsetManager::coordinator(std::shared_ptr<Broker> &broker)
{

    std::unique_lock<std::shared_mutex> lock(m_broker_lock);
    if (broker)
        co_return ErrNoError;
    int err = co_await m_client->RefreshCoordinator(m_group);
    if (err != ErrNoError)
    {
        co_return err;
    }

    err = co_await m_client->GetCoordinator(m_group, m_broker);
    if (err != ErrNoError)
        co_return err;

    co_return ErrNoError;
}

void OffsetManager::releaseCoordinator(std::shared_ptr<Broker> &b)
{
    std::unique_lock<std::shared_mutex> lock(m_broker_lock);
    if (m_broker == b)
    {
        m_broker = nullptr;
    }
}

coev::awaitable<void> OffsetManager::mainLoop()
{
    auto interval = m_conf->Consumer.Offsets.AutoCommit.Interval;
    while (!m_closing)
    {
        co_await sleep_for(interval);
        if (m_closing)
            break;
        co_await Commit();
    }
}

coev::awaitable<int> OffsetManager::Commit()
{
    co_await flushToBroker();
    releasePOMs(false);
    co_return 0;
}

coev::awaitable<void> OffsetManager::flushToBroker()
{
    std::shared_ptr<Broker> broker;
    auto err = co_await coordinator(broker);
    if (err != ErrNoError)
    {
        handleError(ErrBrokerNotAvailable);
        co_return;
    }

    OffsetCommitRequest req;
    auto req_err = constructRequest(req);
    if (req_err != ErrNoError)
    {
        co_return;
    }
    ResponsePromise<OffsetCommitResponse> rp;
    err = co_await broker->Send(req, rp);
    if (err)
    {
        handleError(err);
        releaseCoordinator(broker);
        broker->Close();
        co_return;
    }

    err = rp.decode(req.version());
    if (err)
    {
        handleError(err);
        releaseCoordinator(broker);
        broker->Close();
        co_return;
    }

    handleResponse(broker, req, rp.m_response);
}

int OffsetManager::constructRequest(OffsetCommitRequest &req)
{
    req.m_version = 1;
    req.m_consumer_group = m_group;
    req.m_consumer_id = m_member_id;
    req.m_consumer_group_generation = m_generation;

    if (m_conf->Version.IsAtLeast(V0_9_0_0))
        req.m_version = 2;
    if (m_conf->Version.IsAtLeast(V0_11_0_0))
        req.m_version = 3;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
        req.m_version = 4;
    if (m_conf->Version.IsAtLeast(V2_1_0_0))
        req.m_version = 6;
    if (m_conf->Version.IsAtLeast(V2_3_0_0))
    {
        req.m_version = 7;
        req.m_group_instance_id = m_group_instance_id;
    }

    int64_t commitTimestamp = 0;
    if (req.m_version == 1)
    {
        commitTimestamp = ReceiveTime;
    }

    if (req.m_version >= 2 && req.m_version < 5)
    {
        req.m_retention_time = -1;
        if (m_conf->Consumer.Offsets.Retention > std::chrono::milliseconds(0))
        {
            req.m_retention_time = std::chrono::duration_cast<std::chrono::milliseconds>(m_conf->Consumer.Offsets.Retention).count();
        }
    }

    {

        for (auto &topicManagers : m_poms)
        {
            for (auto &pom : topicManagers.second)
            {
                std::unique_lock<std::mutex> pomLock(pom.second->m_lock);
                if (pom.second->m_dirty)
                {
                    req.AddBlockWithLeaderEpoch(pom.second->m_topic, pom.second->m_partition, pom.second->m_offset, pom.second->m_leaderEpoch, commitTimestamp, pom.second->m_metadata);
                }
            }
        }
    }

    if (!req.blocks.empty())
    {
        return INVALID;
    }

    return ErrNoError;
}

void OffsetManager::handleResponse(std::shared_ptr<Broker> broker, const OffsetCommitRequest &req, OffsetCommitResponse &resp)
{

    for (auto &topicManagers : m_poms)
    {
        for (auto &pom : topicManagers.second)
        {
            if (!req.blocks.count(pom.second->m_topic) || !req.blocks.at(pom.second->m_topic).count(pom.second->m_partition))
            {
                continue;
            }

            if (!resp.m_errors.count(pom.second->m_topic))
            {
                pom.second->HandleError(ErrIncompleteResponse);
                continue;
            }
            if (!resp.m_errors.at(pom.second->m_topic).count(pom.second->m_partition))
            {
                pom.second->HandleError(ErrIncompleteResponse);
                continue;
            }

            auto err = resp.m_errors.at(pom.second->m_topic).at(pom.second->m_partition);

            if (err == ErrNoError)
            {
                auto block = req.blocks.at(pom.second->m_topic).at(pom.second->m_partition);
                pom.second->UpdateCommitted(block->m_offset, block->m_metadata);
            }
            else if (err == ErrNotLeaderForPartition || err == ErrLeaderNotAvailable ||
                     err == ErrConsumerCoordinatorNotAvailable || err == ErrNotCoordinatorForConsumer)
            {
                releaseCoordinator(broker);
            }
            else if (err == ErrOffsetMetadataTooLarge || err == ErrInvalidCommitOffsetSize)
            {
                pom.second->HandleError(err);
            }
            else if (err == ErrOffsetsLoadInProgress)
            {
            }
            else if (err == ErrFencedInstancedId)
            {
                pom.second->HandleError(err);
                tryCancelSession();
            }
            else if (err == ErrUnknownTopicOrPartition)
            {
                pom.second->HandleError(err);
                releaseCoordinator(broker);
            }
            else
            {
                pom.second->HandleError(err);
                releaseCoordinator(broker);
            }
        }
    }
}

void OffsetManager::handleError(KError err)
{

    for (auto &topicManagers : m_poms)
    {
        for (auto &pom : topicManagers.second)
        {
            pom.second->HandleError(err);
        }
    }
}

void OffsetManager::asyncClosePOMs()
{

    for (auto &topicManagers : m_poms)
    {
        for (auto &pom : topicManagers.second)
        {
            pom.second->AsyncClose();
        }
    }
}

int OffsetManager::releasePOMs(bool force)
{
    int remaining = 0;

    for (auto it = m_poms.begin(); it != m_poms.end();)
    {
        auto &topic = it->first;
        auto &topicManagers = it->second;
        for (auto part_it = topicManagers.begin(); part_it != topicManagers.end();)
        {
            auto &pom = part_it->second;
            std::unique_lock<std::mutex> pomLock(pom->m_lock);
            bool releaseDue = pom->m_done && (force || !pom->m_dirty);
            pomLock.unlock();

            if (releaseDue)
            {
                pom->Release();
                part_it = topicManagers.erase(part_it);
            }
            else
            {
                ++part_it;
                ++remaining;
            }
        }
        if (topicManagers.empty())
        {
            it = m_poms.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return remaining;
}

std::shared_ptr<PartitionOffsetManager> OffsetManager::findPOM(const std::string &topic, int32_t partition)
{

    if (m_poms.count(topic) && m_poms.at(topic).count(partition))
    {
        return m_poms.at(topic).at(partition);
    }
    return nullptr;
}

void OffsetManager::tryCancelSession()
{
    if (m_session_canceler)
    {
        m_session_canceler();
    }
}

coev::awaitable<int> NewOffsetManagerFromClient(const std::string &group, std::shared_ptr<Client> client, std::shared_ptr<OffsetManager> &om)
{
    return NewOffsetManagerFromClient(group, "", GroupGenerationUndefined, client, nullptr, om);
}

coev::awaitable<int> NewOffsetManagerFromClient(
    const std::string &group, const std::string &memberID, int32_t generation,
    std::shared_ptr<Client> client, std::function<void()> sessionCanceler, std::shared_ptr<OffsetManager> &om)
{

    if (client->Closed())
    {
        co_return ErrClosedClient;
    }

    auto conf = client->GetConfig();
    om = std::make_shared<OffsetManager>(client, conf, group, sessionCanceler, memberID, generation);

    if (!conf->Consumer.Group.InstanceID.empty())
    {
        om->m_group_instance_id = conf->Consumer.Group.InstanceID;
    }

    if (conf->Consumer.Offsets.AutoCommit.Enable)
    {
        om->m_task << om->mainLoop();
    }

    co_return 0;
}