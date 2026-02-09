#pragma once
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "consumer_message.h"
#include "undefined.h"

struct OffsetManager;
struct ConsumerGroup;
struct ConsumerGroupHandler;
struct PartitionOffsetManager;

struct ConsumerGroupSession : std::enable_shared_from_this<ConsumerGroupSession>
{
    std::string m_member_id;
    int32_t m_generation_id;
    std::shared_ptr<ConsumerGroup> m_parent;
    std::shared_ptr<ConsumerGroupHandler> m_handler;
    std::shared_ptr<OffsetManager> m_offsets;
    std::shared_ptr<Context> m_context;
    std::function<void()> m_cancel;
    std::map<std::string, std::vector<int32_t>> m_claims;
    std::atomic<bool> m_hd_dying{false};
    std::atomic<bool> m_hd_dead{false};
    coev::co_task m_task;

    ConsumerGroupSession() = default;
    ConsumerGroupSession(std::shared_ptr<ConsumerGroup> &, const std::string &,
                         int32_t, std::shared_ptr<ConsumerGroupHandler>, std::shared_ptr<OffsetManager>,
                         std::map<std::string, std::vector<int32_t>> &,
                         std::shared_ptr<Context> &, std::function<void()>);

    std::map<std::string, std::vector<int32_t>> get_claims();
    std::string member_id();
    int32_t generation_id();
    void mark_offset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata);
    void commit();
    void reset_offset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata);
    void mark_message(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata);
    std::shared_ptr<Context> get_context();
    coev::awaitable<void> hander_error(std::shared_ptr<PartitionOffsetManager> pom, const std::string &topic, int32_t partition);

    coev::awaitable<int> _release(bool withCleanup);
    coev::awaitable<void> _consume(const std::string &topic, int32_t partition);
    coev::awaitable<void> _heartbeat_loop();
};
