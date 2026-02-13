#include "partition_offset_manager.h"

PartitionOffsetManager::PartitionOffsetManager(
    std::shared_ptr<OffsetManager> parent, const std::string &topic,
    int32_t partition, int32_t leader_epoch, int64_t offset, const std::string &metadata)
    : m_parent(parent), m_topic(topic), m_partition(partition), m_leader_epoch(leader_epoch), m_offset(offset), m_metadata(metadata), m_dirty(false), m_done(false)
{
}

std::pair<int64_t, std::string> PartitionOffsetManager::next_offset()
{
    if (m_offset >= 0)
    {
        return std::make_pair(m_offset, m_metadata);
    }
    return std::make_pair(m_parent->m_conf->Consumer.Offsets.Initial, "");
}
coev::awaitable<void> PartitionOffsetManager::errors(std::shared_ptr<ConsumerError> &err)
{
    co_await m_errors.get(err);
}
void PartitionOffsetManager::mark_offset(int64_t offset, const std::string &metadata)
{
    if (offset > m_offset)
    {
        m_offset = offset;
        m_metadata = metadata;
        m_dirty = true;
    }
}

void PartitionOffsetManager::reset_offset(int64_t offset, const std::string &metadata)
{
    if (offset <= m_offset)
    {
        m_offset = offset;
        m_metadata = metadata;
        m_dirty = true;
    }
}

void PartitionOffsetManager::update_committed(int64_t offset, const std::string &metadata)
{
    if (m_offset == offset && m_metadata == metadata)
    {
        m_dirty = false;
    }
}

void PartitionOffsetManager::async_close()
{
    m_done = true;
}

void PartitionOffsetManager::close()
{
    async_close();
}

void PartitionOffsetManager::handle_error(KError err)
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

void PartitionOffsetManager::release()
{
}