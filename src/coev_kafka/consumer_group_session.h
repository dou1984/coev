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


struct ConsumerGroupSession :  std::enable_shared_from_this<ConsumerGroupSession>
{
    std::shared_ptr<ConsumerGroup> m_parent;
    std::string m_member_id;
    int32_t m_generation_id;
    std::shared_ptr<ConsumerGroupHandler> m_handler;

    std::map<std::string, std::vector<int32_t>> m_claims;
    std::shared_ptr<OffsetManager> m_offsets;
    std::shared_ptr<Context> m_context;
    std::function<void()> m_cancel;

    std::atomic<bool> m_hd_dying{false};
    std::atomic<bool> m_hd_dead{false};
    coev::co_task m_task;
    coev::co_waitgroup m_waitgroup;

    ConsumerGroupSession() = default;
    ConsumerGroupSession(std::shared_ptr<ConsumerGroup> &, const std::string &,
                         int32_t, std::shared_ptr<ConsumerGroupHandler>, std::shared_ptr<OffsetManager>,
                         std::map<std::string, std::vector<int32_t>> &,
                         std::shared_ptr<Context> &, std::function<void()>);

    std::map<std::string, std::vector<int32_t>> GetClaims();
    std::string MemberID();
    int32_t GenerationID();
    void MarkOffset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata);
    void Commit();
    void ResetOffset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata);
    void MarkMessage(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata);
    std::shared_ptr<Context> GetContext();
    coev::awaitable<void> HanderError(std::shared_ptr<PartitionOffsetManager> pom, const std::string &topic, int32_t partition);

    coev::awaitable<int> _Release(bool withCleanup);
    coev::awaitable<void> _Consume(const std::string &topic, int32_t partition);
    coev::awaitable<void> _HeartbeatLoop();
};
