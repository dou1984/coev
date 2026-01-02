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
    auto err = co_await fetchInitialOffset(topic, partition, conf->Metadata.Retry.Max, offset, leaderEpoch, metadata);
    if (err != 0)
    {
        co_return err;
    }

    pom = std::make_shared<PartitionOffsetManager>();
    pom->Parent = shared_from_this();
    pom->Topic = topic;
    pom->Partition = partition;
    pom->LeaderEpoch = leaderEpoch;
    pom->Offset = offset;
    pom->Metadata = metadata;
    co_return ErrNoError;
}

OffsetManager::OffsetManager(
    std::shared_ptr<Client> client,
    std::shared_ptr<Config> conf,
    const std::string &group,
    std::function<void()> sessionCanceler,
    const std::string &memberID,
    int32_t generation)
    : client(client), conf(conf), group(group), sessionCanceler(sessionCanceler), memberID(memberID), generation(generation), closing(false) {}

std::shared_ptr<PartitionOffsetManager> OffsetManager::ManagePartition(const std::string &topic, int32_t partition)
{
    auto pom = std::make_shared<PartitionOffsetManager>(shared_from_this(), topic, partition, 0, conf->ChannelBufferSize, 0, "");

    {
        std::shared_lock<std::shared_mutex> lock(pomsLock);
        if (poms.count(topic) && poms.at(topic).count(partition))
        {
            throw std::runtime_error("That topic/partition is already being managed");
        }
    }

    {
        std::unique_lock<std::shared_mutex> lock(pomsLock);
        if (!poms.count(topic))
        {
            poms[topic] = {};
        }
        poms[topic][partition] = std::dynamic_pointer_cast<PartitionOffsetManager>(pom);
    }

    return pom;
}

coev::awaitable<int> OffsetManager::Close()
{

    if (closing)
        co_return ErrNoError;
    closing = true;
    if (conf->Consumer.Offsets.AutoCommit.Enable)
    {
        closed.set(true);
    }

    asyncClosePOMs();

    if (conf->Consumer.Offsets.AutoCommit.Enable)
    {
        for (int attempt = 0; attempt <= conf->Consumer.Offsets.Retry.Max; ++attempt)
        {
            co_await flushToBroker();
            if (releasePOMs(false) == 0)
                break;
        }
    }

    releasePOMs(true);

    std::unique_lock<std::shared_mutex> lock(brokerLock);
    broker = nullptr;
    co_return ErrNoError;
}

std::chrono::milliseconds OffsetManager::ComputeBackoff(int retries)
{
    if (conf->Metadata.Retry.BackoffFunc)
    {
        return conf->Metadata.Retry.BackoffFunc(retries, conf->Metadata.Retry.Max);
    }
    else
    {
        return conf->Metadata.Retry.Backoff;
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
    auto req = OffsetFetchRequest::NewOffsetFetchRequest(conf->Version, group, partitions);
    std::shared_ptr<OffsetFetchResponse> resp;
    err = co_await broker->FetchOffset(req, resp);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err; // 简化表示 FetchOffset 失败
        }
        releaseCoordinator(broker);
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    }

    auto block = resp->GetBlock(topic, partition);
    if (!block)
    {
        co_return ErrIncompleteResponse;
    }

    switch (block->Err)
    {
    case ErrNoError:
        offset = block->Offset;
        leaderEpoch = block->LeaderEpoch;
        metadata = block->Metadata;
        co_return 0;
    case ErrNotCoordinatorForConsumer:
        if (retries <= 0)
        {
            co_return block->Err;
        }
        releaseCoordinator(broker);
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return block->Err;
        }
        {
            auto backoff = ComputeBackoff(retries);

            closing = true;
            if (sleep_for(std::chrono::milliseconds(backoff)))
            {
                closed.set(true);
            }
        }
        co_return co_await fetchInitialOffset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    default:
        co_return block->Err;
    }
}
coev::awaitable<int> OffsetManager::coordinator(std::shared_ptr<Broker> &broker)
{

    std::unique_lock<std::shared_mutex> lock(brokerLock);
    if (broker)
        co_return ErrNoError;
    int err = co_await client->RefreshCoordinator(group);
    if (err != ErrNoError)
    {
        co_return err;
    }

    err = co_await client->GetCoordinator(group, broker);
    if (err != ErrNoError)
        co_return err;

    co_return ErrNoError;
}

void OffsetManager::releaseCoordinator(std::shared_ptr<Broker> &b)
{
    std::unique_lock<std::shared_mutex> lock(brokerLock);
    if (broker == b)
    {
        broker = nullptr;
    }
}

coev::awaitable<void> OffsetManager::mainLoop()
{
    auto interval = conf->Consumer.Offsets.AutoCommit.Interval;
    while (!closing)
    {
        co_await sleep_for(interval);
        if (closing)
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

    std::unique_lock<std::mutex> brokerLock(broker->m_Lock);
    auto req = constructRequest();
    if (!req)
    {
        co_return;
    }
    std::shared_ptr<OffsetCommitResponse> resp;
    std::shared_ptr<ResponsePromise> rp;
    err = co_await broker->SendOffsetCommit(req, resp, rp);
    if (err)
    {
        handleError(err);
        releaseCoordinator(broker);
        broker->Close();
        co_return;
    }

    err = co_await HandleResponsePromise(req, resp, rp, nullptr);
    if (err)
    {
        handleError(err);
        releaseCoordinator(broker);
        broker->Close();
        co_return;
    }

    broker->UandleThrottledResponse(resp);
    handleResponse(broker, req, resp);
}

std::shared_ptr<OffsetCommitRequest> OffsetManager::constructRequest()
{
    auto r = std::make_unique<OffsetCommitRequest>();
    r->Version = 1;
    r->ConsumerGroup = group;
    r->ConsumerID = memberID;
    r->ConsumerGroupGeneration = generation;

    if (conf->Version.IsAtLeast(V0_9_0_0))
        r->Version = 2;
    if (conf->Version.IsAtLeast(V0_11_0_0))
        r->Version = 3;
    if (conf->Version.IsAtLeast(V2_0_0_0))
        r->Version = 4;
    if (conf->Version.IsAtLeast(V2_1_0_0))
        r->Version = 6;
    if (conf->Version.IsAtLeast(V2_3_0_0))
    {
        r->Version = 7;
        r->GroupInstanceId = groupInstanceId;
    }

    int64_t commitTimestamp = 0;
    if (r->Version == 1)
    {
        commitTimestamp = ReceiveTime;
    }

    if (r->Version >= 2 && r->Version < 5)
    {
        r->RetentionTime = -1;
        if (conf->Consumer.Offsets.Retention > std::chrono::milliseconds(0))
        {
            r->RetentionTime = std::chrono::duration_cast<std::chrono::milliseconds>(conf->Consumer.Offsets.Retention).count();
        }
    }

    {
        std::shared_lock<std::shared_mutex> lock(pomsLock);
        for (auto &topicManagers : poms)
        {
            for (auto &pom : topicManagers.second)
            {
                std::unique_lock<std::mutex> pomLock(pom.second->Lock);
                if (pom.second->dirty)
                {
                    r->AddBlockWithLeaderEpoch(pom.second->Topic, pom.second->Partition, pom.second->Offset, pom.second->LeaderEpoch, commitTimestamp, pom.second->Metadata);
                }
            }
        }
    }

    if (!r->blocks.empty())
    {
        return r;
    }

    return nullptr;
}

void OffsetManager::handleResponse(std::shared_ptr<Broker> broker, std::shared_ptr<OffsetCommitRequest> req, std::shared_ptr<OffsetCommitResponse> resp)
{
    std::shared_lock<std::shared_mutex> lock(pomsLock);
    for (auto &topicManagers : poms)
    {
        for (auto &pom : topicManagers.second)
        {
            if (!req->blocks.count(pom.second->Topic) || !req->blocks.at(pom.second->Topic).count(pom.second->Partition))
            {
                continue;
            }

            if (!resp->Errors.count(pom.second->Topic))
            {
                pom.second->HandleError(ErrIncompleteResponse);
                continue;
            }
            if (!resp->Errors.at(pom.second->Topic).count(pom.second->Partition))
            {
                pom.second->HandleError(ErrIncompleteResponse);
                continue;
            }

            auto err = resp->Errors.at(pom.second->Topic).at(pom.second->Partition);

            if (err == ErrNoError)
            {
                auto block = req->blocks.at(pom.second->Topic).at(pom.second->Partition);
                pom.second->UpdateCommitted(block->Offset, block->Metadata);
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
    std::shared_lock<std::shared_mutex> lock(pomsLock);
    for (auto &topicManagers : poms)
    {
        for (auto &pom : topicManagers.second)
        {
            pom.second->HandleError(err);
        }
    }
}

void OffsetManager::asyncClosePOMs()
{
    std::shared_lock<std::shared_mutex> lock(pomsLock);
    for (auto &topicManagers : poms)
    {
        for (auto &pom : topicManagers.second)
        {
            pom.second->AsyncClose();
        }
    }
}

int OffsetManager::releasePOMs(bool force)
{
    std::unique_lock<std::shared_mutex> lock(pomsLock);
    int remaining = 0;

    for (auto it = poms.begin(); it != poms.end();)
    {
        auto &topic = it->first;
        auto &topicManagers = it->second;
        for (auto part_it = topicManagers.begin(); part_it != topicManagers.end();)
        {
            auto &pom = part_it->second;
            std::unique_lock<std::mutex> pomLock(pom->Lock);
            bool releaseDue = pom->done && (force || !pom->dirty);
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
            it = poms.erase(it);
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
    std::shared_lock<std::shared_mutex> lock(pomsLock);
    if (poms.count(topic) && poms.at(topic).count(partition))
    {
        return poms.at(topic).at(partition);
    }
    return nullptr;
}

void OffsetManager::tryCancelSession()
{
    if (sessionCanceler)
    {
        sessionCanceler();
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
        om->groupInstanceId = conf->Consumer.Group.InstanceID;
    }

    if (conf->Consumer.Offsets.AutoCommit.Enable)
    {
        om->task_ << om->mainLoop();
    }

    co_return 0;
}