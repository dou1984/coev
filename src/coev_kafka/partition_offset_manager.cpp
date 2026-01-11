#include "partition_offset_manager.h"

PartitionOffsetManager::PartitionOffsetManager(
    std::shared_ptr<OffsetManager> parent, const std::string &topic,
    int32_t partition, int32_t leaderEpoch, size_t channelBufferSize, int64_t offset, const std::string &metadata)
    : m_parent(parent), m_topic(topic), m_partition(partition), m_leaderEpoch(leaderEpoch), m_offset(offset), m_metadata(metadata), m_dirty(false), m_done(false)
{
}

std::pair<int64_t, std::string> PartitionOffsetManager::NextOffset()
{
    std::unique_lock<std::mutex> l(m_lock);
    if (m_offset >= 0)
    {
        return std::make_pair(m_offset, m_metadata);
    }
    return std::make_pair(m_parent->m_conf->Consumer.Offsets.Initial, "");
}
coev::co_channel<std::shared_ptr<ConsumerError>> &PartitionOffsetManager::Errors()
{
    return m_errors;
}
void PartitionOffsetManager::MarkOffset(int64_t offset, const std::string &metadata)
{
    std::unique_lock<std::mutex> l(m_lock);
    if (offset > m_offset)
    {
        m_offset = offset;
        m_metadata = metadata;
        m_dirty = true;
    }
}

void PartitionOffsetManager::ResetOffset(int64_t offset, const std::string &metadata)
{
    std::unique_lock<std::mutex> l(m_lock);
    if (offset <= m_offset)
    {
        m_offset = offset;
        m_metadata = metadata;
        m_dirty = true;
    }
}

void PartitionOffsetManager::UpdateCommitted(int64_t offset, const std::string &metadata)
{
    std::unique_lock<std::mutex> l(m_lock);
    if (m_offset == offset && m_metadata == metadata)
    {
        m_dirty = false;
    }
}

void PartitionOffsetManager::AsyncClose()
{
    std::unique_lock<std::mutex> l(m_lock);
    m_done = true;
}

void PartitionOffsetManager::Close()
{
    AsyncClose();
}

void PartitionOffsetManager::HandleError(KError err)
{
    auto cErr = std::make_shared<ConsumerError>();
    cErr->m_topic = m_topic;
    cErr->m_partition = m_partition;
    cErr->m_err = err;

    if (m_parent->m_conf->Consumer.Return.Errors)
    {
        m_errors.set(cErr);
    }
}

void PartitionOffsetManager::Release()
{
}