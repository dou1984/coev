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

struct OffsetManager : std::enable_shared_from_this<OffsetManager>
{
    coev::awaitable<int> NewPartitionOffsetManager(const std::string &topic, int32_t partition, std::shared_ptr<PartitionOffsetManager> &pom);
    coev::awaitable<int> Close();
    coev::awaitable<int> Commit();

    std::shared_ptr<PartitionOffsetManager> ManagePartition(const std::string &topic, int32_t partition);
    OffsetManager(std::shared_ptr<Client> client, std::shared_ptr<Config> conf, const std::string &group, std::function<void()> sessionCanceler, const std::string &memberID, int32_t generation);
    OffsetManager(std::shared_ptr<Client> client, const std::string &group, const std::string &memberID, int32_t generation, std::function<void()> sessionCanceler);
    std::chrono::milliseconds ComputeBackoff(int retries);
    coev::awaitable<int> FetchInitialOffset(const std::string &topic, int32_t partition, int retries, int64_t &offset, int32_t &leaderEpoch, std::string &metadata);
    coev::awaitable<int> Coordinator(std::shared_ptr<Broker> &);
    coev::awaitable<void> MainLoop();
    coev::awaitable<void> FlushToBroker();
    int ConstructRequest(OffsetCommitRequest &req);
    void ReleaseCoordinator(std::shared_ptr<Broker> &broker);
    void HandleResponse(std::shared_ptr<Broker> broker, const OffsetCommitRequest &req, OffsetCommitResponse &resp);
    void HandleError(KError err);
    void HandleError(int err) { HandleError((KError)err); }
    void AsyncClosePOMs();
    int ReleasePOMs(bool force);
    std::shared_ptr<PartitionOffsetManager> FindPOM(const std::string &topic, int32_t partition);
    void TryCancelSession();

    std::shared_ptr<Client> m_client;
    std::shared_ptr<Config> m_conf;
    std::string m_group;
    std::function<void()> m_session_canceler;

    std::string m_member_id;
    std::string m_group_instance_id;
    int32_t m_generation;

    std::shared_ptr<Broker> m_broker;

    std::unordered_map<std::string, std::map<int32_t, std::shared_ptr<PartitionOffsetManager>>> m_poms;
    std::atomic<bool> m_closing = false;

    coev::co_channel<bool> m_closed;
    coev::co_task m_task;
};


