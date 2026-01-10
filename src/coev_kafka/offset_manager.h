#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <coev/coev.h>
#include "version.h"
#include "client.h"
#include "broker.h"
#include "consumer.h"
#include "offset_commit_request.h"
#include "offset_commit_response.h"
#include "protocol_body.h"
#include "undefined.h"

struct PartitionOffsetManager;

struct IOffsetManager
{
    virtual ~IOffsetManager() = default;
    virtual std::shared_ptr<PartitionOffsetManager> ManagePartition(const std::string &topic, int32_t partition) = 0;
    virtual coev::awaitable<int> Close() = 0;
    virtual coev::awaitable<int> Commit() = 0;
};

struct OffsetManager : IOffsetManager, std::enable_shared_from_this<OffsetManager>
{
    coev::awaitable<int> NewPartitionOffsetManager(const std::string &topic, int32_t partition, std::shared_ptr<PartitionOffsetManager> &pom);
    coev::awaitable<int> Close();
    coev::awaitable<int> Commit();

    std::shared_ptr<PartitionOffsetManager> ManagePartition(const std::string &topic, int32_t partition);
    OffsetManager(std::shared_ptr<Client> client, std::shared_ptr<Config> conf,
                  const std::string &group, std::function<void()> sessionCanceler,
                  const std::string &memberID, int32_t generation);
    std::chrono::milliseconds ComputeBackoff(int retries);
    coev::awaitable<int> fetchInitialOffset(const std::string &topic, int32_t partition, int retries, int64_t &offset, int32_t &leaderEpoch, std::string &metadata);
    coev::awaitable<int> coordinator(std::shared_ptr<Broker> &);
    coev::awaitable<void> mainLoop();
    coev::awaitable<void> flushToBroker();
    std::shared_ptr<OffsetCommitRequest> constructRequest();
    void releaseCoordinator(std::shared_ptr<Broker> &b);
    void handleResponse(std::shared_ptr<Broker> broker, std::shared_ptr<OffsetCommitRequest> req, std::shared_ptr<OffsetCommitResponse> resp);
    void handleError(KError err);
    void handleError(int err) { handleError((KError)err); }
    void asyncClosePOMs();
    int releasePOMs(bool force);
    std::shared_ptr<PartitionOffsetManager> findPOM(const std::string &topic, int32_t partition);
    void tryCancelSession();

    std::shared_ptr<Client> m_client;
    std::shared_ptr<Config> m_conf;
    std::string m_group;
    std::function<void()> m_session_canceler;

    std::string m_member_id;
    std::string m_group_instance_id;
    int32_t m_generation;

    std::shared_mutex m_broker_lock;
    std::shared_ptr<Broker> m_broker;

    std::shared_mutex m_poms_lock;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionOffsetManager>>> m_poms;

    std::atomic<bool> m_closing = false;

    coev::co_channel<bool> m_closed;
    coev::co_task m_task;
};

coev::awaitable<int> NewOffsetManagerFromClient(const std::string &group, std::shared_ptr<Client> client, std::shared_ptr<OffsetManager> &om);
coev::awaitable<int> NewOffsetManagerFromClient(const std::string &group, const std::string &memberID, int32_t generation,
                                                std::shared_ptr<Client> client, std::function<void()> sessionCanceler, std::shared_ptr<OffsetManager> &om);
