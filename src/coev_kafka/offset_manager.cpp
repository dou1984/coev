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

coev::awaitable<int> OffsetManager::create_partition_offset_manager(const std::string &topic, int32_t partition, std::shared_ptr<PartitionOffsetManager> &pom)
{

    int64_t offset;
    int32_t leader_epoch;
    std::string metadata;
    auto err = co_await fetch_initial_offset(topic, partition, m_conf->Metadata.Retry.Max, offset, leader_epoch, metadata);
    if (err != 0)
    {
        co_return err;
    }

    pom = std::make_shared<PartitionOffsetManager>();
    pom->m_parent = shared_from_this();
    pom->m_topic = topic;
    pom->m_partition = partition;
    pom->m_leader_epoch = leader_epoch;
    pom->m_offset = offset;
    pom->m_metadata = metadata;
    co_return ErrNoError;
}

OffsetManager::OffsetManager(std::shared_ptr<Client> client, std::shared_ptr<Config> conf, const std::string &group,
                             std::function<void()> sessionCanceler, const std::string &memberID, int32_t generation)
    : m_client(client), m_conf(conf), m_group(group), m_session_canceler(sessionCanceler), m_member_id(memberID), m_generation(generation), m_closing(false)
{
    if (!m_conf->Consumer.Group.InstanceID.empty())
    {
        m_group_instance_id = m_conf->Consumer.Group.InstanceID;
    }

    if (m_conf->Consumer.Offsets.AutoCommit.Enable)
    {
        m_task << main_loop();
    }
}

OffsetManager::OffsetManager(std::shared_ptr<Client> client, const std::string &group, const std::string &memberID, int32_t generation, std::function<void()> sessionCanceler)
{
    if (client->Closed())
    {
        throw std::runtime_error("Client is closed");
    }

    auto conf = client->GetConfig();
    m_client = client;
    m_conf = conf;
    m_group = group;
    m_session_canceler = sessionCanceler;
    m_member_id = memberID;
    m_generation = generation;
    m_closing = false;

    if (!conf->Consumer.Group.InstanceID.empty())
    {
        m_group_instance_id = conf->Consumer.Group.InstanceID;
    }

    if (conf->Consumer.Offsets.AutoCommit.Enable)
    {
        m_task << main_loop();
    }
}

std::shared_ptr<PartitionOffsetManager> OffsetManager::manage_partition(const std::string &topic, int32_t partition)
{
    auto pom = std::make_shared<PartitionOffsetManager>(shared_from_this(), topic, partition, 0, m_conf->ChannelBufferSize, "");
    {
        if (m_poms.count(topic) && m_poms.at(topic).count(partition))
        {
            throw std::runtime_error("That topic/partition is already being managed");
        }
    }
    m_poms[topic][partition] = pom;
    return pom;
}

coev::awaitable<int> OffsetManager::close()
{

    if (m_closing)
    {
        co_return ErrNoError;
    }
    m_closing = true;
    if (m_conf->Consumer.Offsets.AutoCommit.Enable)
    {
        m_closed.set(true);
    }

    async_close_poms();

    if (m_conf->Consumer.Offsets.AutoCommit.Enable)
    {
        for (int attempt = 0; attempt <= m_conf->Consumer.Offsets.Retry.Max; ++attempt)
        {
            co_await flush_to_broker();
            if (release_poms(false) == 0)
            {
                break;
            }
        }
    }

    release_poms(true);

    m_broker = nullptr;
    co_return ErrNoError;
}

std::chrono::milliseconds OffsetManager::compute_backoff(int retries)
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

coev::awaitable<int> OffsetManager::fetch_initial_offset(const std::string &topic, int32_t partition, int retries, int64_t &offset, int32_t &leaderEpoch, std::string &metadata)
{
    std::shared_ptr<Broker> broker;
    auto err = co_await coordinator(broker);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err;
        }
        co_return co_await fetch_initial_offset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    }

    std::map<std::string, std::vector<int32_t>> partitions;
    partitions[topic] = {partition};
    auto req = std::make_shared<OffsetFetchRequest>(m_conf->Version, m_group, partitions);
    ResponsePromise<OffsetFetchResponse> resp;
    err = co_await broker->FetchOffset(req, resp);
    if (err != 0)
    {
        if (retries <= 0)
        {
            co_return err;
        }
        release_coordinator(broker);
        co_return co_await fetch_initial_offset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    }

    auto block = resp.m_response->get_block(topic, partition);
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
        release_coordinator(broker);
        co_return co_await fetch_initial_offset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    case ErrOffsetsLoadInProgress:
        if (retries <= 0)
        {
            co_return block->m_err;
        }
        {
            auto backoff = compute_backoff(retries);

            m_closing = true;
            if (sleep_for(std::chrono::milliseconds(backoff)))
            {
                m_closed.set(true);
            }
        }
        co_return co_await fetch_initial_offset(topic, partition, retries - 1, offset, leaderEpoch, metadata);
    default:
        co_return block->m_err;
    }
}
coev::awaitable<int> OffsetManager::coordinator(std::shared_ptr<Broker> &broker)
{

    if (broker)
    {
        co_return ErrNoError;
    }
    int err = co_await m_client->RefreshCoordinator(m_group);
    if (err != ErrNoError)
    {
        co_return err;
    }

    err = co_await m_client->GetCoordinator(m_group, m_broker);
    if (err != ErrNoError)
    {
        co_return err;
    }

    co_return ErrNoError;
}

void OffsetManager::release_coordinator(std::shared_ptr<Broker> &broker)
{
    if (m_broker == broker)
    {
        m_broker = nullptr;
    }
}

coev::awaitable<void> OffsetManager::main_loop()
{
    auto interval = m_conf->Consumer.Offsets.AutoCommit.Interval;
    while (!m_closing)
    {
        co_await sleep_for(interval);
        if (m_closing)
            break;
        co_await commit();
    }
}

coev::awaitable<int> OffsetManager::commit()
{
    co_await flush_to_broker();
    release_poms(false);
    co_return 0;
}

coev::awaitable<void> OffsetManager::flush_to_broker()
{
    std::shared_ptr<Broker> broker;
    auto err = co_await coordinator(broker);
    if (err != ErrNoError)
    {
        handle_error(ErrBrokerNotAvailable);
        co_return;
    }

    auto request = std::make_shared<OffsetCommitRequest>();
    err = construct_request(*request);
    if (err != ErrNoError)
    {
        co_return;
    }
    ResponsePromise<OffsetCommitResponse> response;
    err = co_await broker->SendAndReceive(request, response);
    if (err)
    {
        handle_error(err);
        release_coordinator(broker);
        broker->Close();
        co_return;
    }

    err = response.decode(request->version());
    if (err)
    {
        handle_error(err);
        release_coordinator(broker);
        broker->Close();
        co_return;
    }

    handle_response(broker, *request, *response.m_response);
}

int OffsetManager::construct_request(OffsetCommitRequest &request)
{
    request.m_version = 1;
    request.m_consumer_group = m_group;
    request.m_consumer_id = m_member_id;
    request.m_consumer_group_generation = m_generation;

    if (m_conf->Version.IsAtLeast(V0_9_0_0))
        request.m_version = 2;
    if (m_conf->Version.IsAtLeast(V0_11_0_0))
        request.m_version = 3;
    if (m_conf->Version.IsAtLeast(V2_0_0_0))
        request.m_version = 4;
    if (m_conf->Version.IsAtLeast(V2_1_0_0))
        request.m_version = 6;
    if (m_conf->Version.IsAtLeast(V2_3_0_0))
    {
        request.m_version = 7;
        request.m_group_instance_id = m_group_instance_id;
    }

    int64_t commit_timestamp = 0;
    if (request.m_version == 1)
    {
        commit_timestamp = ReceiveTime;
    }

    if (request.m_version >= 2 && request.m_version < 5)
    {
        request.m_retention_time = -1;
        if (m_conf->Consumer.Offsets.Retention > std::chrono::milliseconds(0))
        {
            request.m_retention_time = std::chrono::duration_cast<std::chrono::milliseconds>(m_conf->Consumer.Offsets.Retention).count();
        }
    }

    for (auto &topic_managers : m_poms)
    {
        for (auto &pom : topic_managers.second)
        {
            if (pom.second->m_dirty)
            {
                request.add_block_with_leader_epoch(pom.second->m_topic, pom.second->m_partition, pom.second->m_offset, pom.second->m_leader_epoch, commit_timestamp, pom.second->m_metadata);
            }
        }
    }

    if (!request.m_blocks.empty())
    {
        return INVALID;
    }

    return ErrNoError;
}

void OffsetManager::handle_response(std::shared_ptr<Broker> broker, const OffsetCommitRequest &req, OffsetCommitResponse &resp)
{

    for (auto &topic_managers : m_poms)
    {
        for (auto &pom : topic_managers.second)
        {
            if (!req.m_blocks.count(pom.second->m_topic) || !req.m_blocks.at(pom.second->m_topic).count(pom.second->m_partition))
            {
                continue;
            }

            if (!resp.m_errors.count(pom.second->m_topic))
            {
                pom.second->handle_error(ErrIncompleteResponse);
                continue;
            }
            if (!resp.m_errors.at(pom.second->m_topic).count(pom.second->m_partition))
            {
                pom.second->handle_error(ErrIncompleteResponse);
                continue;
            }

            auto err = resp.m_errors.at(pom.second->m_topic).at(pom.second->m_partition);

            if (err == ErrNoError)
            {
                auto block = req.m_blocks.at(pom.second->m_topic).at(pom.second->m_partition);
                pom.second->update_committed(block.m_offset, block.m_metadata);
            }
            else if (err == ErrNotLeaderForPartition || err == ErrLeaderNotAvailable ||
                     err == ErrConsumerCoordinatorNotAvailable || err == ErrNotCoordinatorForConsumer)
            {
                release_coordinator(broker);
            }
            else if (err == ErrOffsetMetadataTooLarge || err == ErrInvalidCommitOffsetSize)
            {
                pom.second->handle_error(err);
            }
            else if (err == ErrOffsetsLoadInProgress)
            {
            }
            else if (err == ErrFencedInstancedId)
            {
                pom.second->handle_error(err);
                try_cancel_session();
            }
            else if (err == ErrUnknownTopicOrPartition)
            {
                pom.second->handle_error(err);
                release_coordinator(broker);
            }
            else
            {
                pom.second->handle_error(err);
                release_coordinator(broker);
            }
        }
    }
}

void OffsetManager::handle_error(KError err)
{

    for (auto &topic_managers : m_poms)
    {
        for (auto &pom : topic_managers.second)
        {
            pom.second->handle_error(err);
        }
    }
}

void OffsetManager::async_close_poms()
{

    for (auto &topic_managers : m_poms)
    {
        for (auto &pom : topic_managers.second)
        {
            pom.second->async_close();
        }
    }
}

int OffsetManager::release_poms(bool force)
{
    int remaining = 0;

    for (auto it = m_poms.begin(); it != m_poms.end();)
    {
        auto &topic = it->first;
        auto &topic_managers = it->second;
        for (auto pit = topic_managers.begin(); pit != topic_managers.end();)
        {
            auto &pom = pit->second;
            bool releaseDue = pom->m_done && (force || !pom->m_dirty);

            if (releaseDue)
            {
                pom->release();
                pit = topic_managers.erase(pit);
            }
            else
            {
                ++pit;
                ++remaining;
            }
        }
        if (topic_managers.empty())
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

std::shared_ptr<PartitionOffsetManager> OffsetManager::find_pom(const std::string &topic, int32_t partition)
{

    if (m_poms.count(topic) && m_poms.at(topic).count(partition))
    {
        return m_poms.at(topic).at(partition);
    }
    return nullptr;
}

void OffsetManager::try_cancel_session()
{
    if (m_session_canceler)
    {
        m_session_canceler();
    }
}
