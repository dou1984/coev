#pragma once
#include <memory>
#include <coev/coev.h>
#include "offset_manager.h"
#include "consumer.h"
#include "consumer_error.h"

struct PartitionOffsetManager
{
    PartitionOffsetManager() = default;
    PartitionOffsetManager(std::shared_ptr<OffsetManager> parent, const std::string &topic, int32_t partition,
                           int32_t leaderEpoch, size_t channelBufferSize, int64_t offset, const std::string &metadata);

    std::pair<int64_t, std::string> NextOffset();
    coev::awaitable<void> Errors(std::shared_ptr<ConsumerError> &err);
    void MarkOffset(int64_t offset, const std::string &metadata);
    void ResetOffset(int64_t offset, const std::string &metadata);
    void AsyncClose();
    void Close();

    void HandleError(KError err);
    void UpdateCommitted(int64_t offset, const std::string &metadata);
    void Release();

    std::shared_ptr<OffsetManager> m_parent;
    std::string m_topic;
    int32_t m_partition;
    int32_t m_leaderEpoch;

    std::mutex m_lock;
    int64_t m_offset;
    std::string m_metadata;
    bool m_dirty;
    bool m_done;

    coev::co_channel<std::shared_ptr<ConsumerError>> m_errors;
};
