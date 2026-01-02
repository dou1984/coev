#pragma once
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "consumer_message.h"
#include "undefined.h"

struct IOffsetManager;
struct ConsumerGroup;
struct ConsumerGroupHandler;

struct IConsumerGroupSession
{
    virtual ~IConsumerGroupSession() = default;
    virtual std::map<std::string, std::vector<int32_t>> GetClaims() = 0;
    virtual std::string MemberID() = 0;
    virtual int32_t GenerationID() = 0;
    virtual void MarkOffset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata) = 0;
    virtual void Commit() = 0;
    virtual void ResetOffset(const std::string &topic, int32_t partition, int64_t offset, const std::string &metadata) = 0;
    virtual void MarkMessage(std::shared_ptr<ConsumerMessage> msg, const std::string &metadata) = 0;
    virtual std::shared_ptr<Context> GetContext() = 0;
};

struct ConsumerGroupSession : IConsumerGroupSession, std::enable_shared_from_this<ConsumerGroupSession>
{
    std::shared_ptr<ConsumerGroup> parent;
    std::string memberID;
    int32_t generationID;
    std::shared_ptr<ConsumerGroupHandler> handler;

    std::map<std::string, std::vector<int32_t>> claims;
    std::shared_ptr<IOffsetManager> offsets;
    std::shared_ptr<Context> context;
    std::function<void()> cancel;

    std::atomic<bool> hbDying{false};
    std::atomic<bool> hbDead{false};
    coev::co_task task_;
    coev::co_waitgroup waitgroup_;

    ConsumerGroupSession() = default;
    ConsumerGroupSession(std::shared_ptr<ConsumerGroup> &, const std::string &,
                         int32_t, std::shared_ptr<ConsumerGroupHandler>, std::shared_ptr<IOffsetManager>,
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

    coev::awaitable<int> Release_(bool withCleanup);
    coev::awaitable<void> Consume_(const std::string &topic, int32_t partition);
    coev::awaitable<void> HeartbeatLoop_();
};
