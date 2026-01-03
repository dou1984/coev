#pragma once
#include <memory>
#include <coev/coev.h>
#include "offset_manager.h"
#include "consumer.h"
#include "consumer_error.h"

struct IPartitionOffsetManager
{
    virtual ~IPartitionOffsetManager() = default;
    virtual std::pair<int64_t, std::string> NextOffset() = 0;
    virtual void MarkOffset(int64_t offset, const std::string &metadata) = 0;
    virtual void ResetOffset(int64_t offset, const std::string &metadata) = 0;
    virtual void AsyncClose() = 0;
    virtual void Close() = 0;
    virtual coev::co_channel<std::shared_ptr<ConsumerError>> &Errors() = 0;
};

struct PartitionOffsetManager : IPartitionOffsetManager
{
    PartitionOffsetManager() = default;

    PartitionOffsetManager(std::shared_ptr<OffsetManager> parent, const std::string &topic, int32_t partition,
                           int32_t leaderEpoch, size_t channelBufferSize, int64_t offset, const std::string &metadata);

    std::pair<int64_t, std::string> NextOffset();
    coev::co_channel<std::shared_ptr<ConsumerError>> &Errors();
    void MarkOffset(int64_t offset, const std::string &metadata);
    void ResetOffset(int64_t offset, const std::string &metadata);
    void AsyncClose();
    void Close();

    void HandleError(KError err);
    void UpdateCommitted(int64_t offset, const std::string &metadata);
    void Release();

    std::shared_ptr<OffsetManager> Parent;
    std::string Topic;
    int32_t Partition;
    int32_t LeaderEpoch;

    std::mutex Lock;
    int64_t Offset;
    std::string Metadata;
    bool dirty;
    bool done;

    coev::co_channel<std::shared_ptr<ConsumerError>> errors;
};
