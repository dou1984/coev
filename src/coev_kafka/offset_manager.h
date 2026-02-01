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
    OffsetManager(std::shared_ptr<Client> client, std::shared_ptr<Config> conf, const std::string &group, std::function<void()> sessionCanceler, const std::string &memberID, int32_t generation);
    OffsetManager(std::shared_ptr<Client> client, const std::string &group, const std::string &memberID, int32_t generation, std::function<void()> sessionCanceler);

    coev::awaitable<int> create_partition_offset_manager(const std::string &topic, int32_t partition, std::shared_ptr<PartitionOffsetManager> &pom);
    coev::awaitable<int> close();
    coev::awaitable<int> commit();

    std::shared_ptr<PartitionOffsetManager> manage_partition(const std::string &topic, int32_t partition);
    std::chrono::milliseconds compute_backoff(int retries);
    coev::awaitable<int> fetch_initial_offset(const std::string &topic, int32_t partition, int retries, int64_t &offset, int32_t &leaderEpoch, std::string &metadata);
    coev::awaitable<int> coordinator(std::shared_ptr<Broker> &);
    coev::awaitable<void> main_loop();
    coev::awaitable<void> flush_to_broker();
    int construct_request(OffsetCommitRequest &req);
    void release_coordinator(std::shared_ptr<Broker> &broker);
    void handle_response(std::shared_ptr<Broker> broker, const OffsetCommitRequest &req, OffsetCommitResponse &resp);
    void handle_error(KError err);
    void handle_error(int err) { handle_error((KError)err); }
    void async_close_poms();
    int release_poms(bool force);
    std::shared_ptr<PartitionOffsetManager> find_pom(const std::string &topic, int32_t partition);
    void try_cancel_session();

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
