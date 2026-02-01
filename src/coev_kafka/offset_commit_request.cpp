#include <iostream>
#include "version.h"
#include "api_versions.h"
#include "offset_commit_request.h"

int OffsetCommitRequestBlock::encode(packet_encoder &pe, int16_t version) const
{
    pe.putInt64(m_offset);

    if (version == 1)
    {
        pe.putInt64(m_timestamp);
    }
    else if (m_timestamp != 0)
    {
    }
    if (version >= 6)
    {
        pe.putInt32(m_committed_leader_epoch);
    }
    return pe.putString(m_metadata);
}

int OffsetCommitRequestBlock::decode(packet_decoder &pd, int16_t version)
{
    auto err = pd.getInt64(m_offset);
    if (err != 0)
        return err;

    if (version == 1)
    {
        err = pd.getInt64(m_timestamp);
        if (err != 0)
            return err;
    }
    if (version >= 6)
    {
        err = pd.getInt32(m_committed_leader_epoch);
        if (err != 0)
            return err;
    }
    err = pd.getString(m_metadata);
    return err;
}

OffsetCommitRequest::OffsetCommitRequest()
    : m_consumer_group_generation(0), m_retention_time(0), m_version(0)
{
}

void OffsetCommitRequest::set_version(int16_t v)
{
    m_version = v;
}

int OffsetCommitRequest::encode(packet_encoder &pe) const
{
    if (m_version < 0 || m_version > 7)
    {
        return ErrConsumerOffsetNotAdvanced;
    }

    auto err = pe.putString(m_consumer_group);

    if (m_version >= 1)
    {
        pe.putInt32(m_consumer_group_generation);
        pe.putString(m_consumer_id);
    }
    else
    {
        if (m_consumer_group_generation != 0)
        {
        }
        if (!m_consumer_id.empty())
        {
        }
    }

    if (m_version >= 2 && m_version <= 4)
    {
        pe.putInt64(m_retention_time);
    }
    else if (m_retention_time != 0)
    {
    }

    if (m_version >= 7)
    {
        pe.putNullableString(m_group_instance_id);
    }

    pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    for (auto &topicEntry : m_blocks)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partitionEntry : partitions)
        {
            pe.putInt32(partitionEntry.first);
            partitionEntry.second.encode(pe, m_version);
        }
    }
    return 0;
}

int OffsetCommitRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    auto err = pd.getString(m_consumer_group);
    if (err != 0)
        return err;

    if (m_version >= 1)
    {
        pd.getInt32(m_consumer_group_generation);
        pd.getString(m_consumer_id);
    }

    if (m_version >= 2 && m_version <= 4)
    {
        pd.getInt64(m_retention_time);
    }

    if (m_version >= 7)
    {
        pd.getNullableString(m_group_instance_id);
    }

    int32_t topicCount;
    err = pd.getArrayLength(topicCount);
    if (topicCount == 0)
    {
        return err;
    }

    m_blocks.clear();
    for (int i = 0; i < topicCount; ++i)
    {
        std::string topic;
        err = pd.getString(topic);
        if (err != 0)
        {
            return err;
        }
        int32_t partitionCount;
        err = pd.getArrayLength(partitionCount);
        if (partitionCount == 0)
        {
            return err;
        }
        auto &partitionMap = m_blocks[topic];
        for (int j = 0; j < partitionCount; ++j)
        {
            int32_t partition;
            err = pd.getInt32(partition);
            if (err != 0)
            {
                return err;
            }
            partitionMap[partition].decode(pd, m_version);
        }
    }
    return 0;
}

int16_t OffsetCommitRequest::key() const
{
    return apiKeyOffsetCommit;
}
int16_t OffsetCommitRequest::version() const
{
    return m_version;
}

int16_t OffsetCommitRequest::header_version() const
{
    return 1;
}

bool OffsetCommitRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 7;
}

KafkaVersion OffsetCommitRequest::required_version() const
{
    switch (m_version)
    {
    case 7:
        return V2_3_0_0;
    case 5:
    case 6:
        return V2_1_0_0;
    case 4:
        return V2_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_9_0_0;
    case 0:
    case 1:
        return V0_8_2_0;
    default:
        return V2_4_0_0;
    }
}

void OffsetCommitRequest::add_block(const std::string &topic, int32_t partitionID, int64_t offset, int64_t timestamp, const std::string &metadata)
{
    add_block_with_leader_epoch(topic, partitionID, offset, 0, timestamp, metadata);
}

void OffsetCommitRequest::add_block_with_leader_epoch(const std::string &topic, int32_t partitionID, int64_t offset, int32_t leaderEpoch, int64_t timestamp, const std::string &metadata)
{
    m_blocks[topic].emplace(partitionID, OffsetCommitRequestBlock(offset, timestamp, leaderEpoch, metadata));
}

std::pair<int64_t, std::string> OffsetCommitRequest::offset(const std::string &topic, int32_t partitionID) const
{
    auto tit = m_blocks.find(topic);
    if (tit == m_blocks.end())
    {
        throw std::runtime_error("no such offset");
    }
    auto &partitions = tit->second;
    auto pit = partitions.find(partitionID);
    if (pit == partitions.end())
    {
        throw std::runtime_error("no such offset");
    }
    auto &block = pit->second;
    return std::make_pair(block.m_offset, block.m_metadata);
}