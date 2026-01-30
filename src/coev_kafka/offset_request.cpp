#include "version.h"
#include "offset_request.h"
#include <stdexcept>

OffsetRequestBlock::OffsetRequestBlock()
    : m_leader_epoch(-1), m_timestamp(0), m_max_num_offsets(0) {}

int OffsetRequestBlock::encode(packetEncoder &pe, int16_t version)
{
    if (version >= 4)
    {
        pe.putInt32(m_leader_epoch);
    }

    pe.putInt64(m_timestamp);

    if (version == 0)
    {
        pe.putInt32(m_max_num_offsets);
    }

    return 0;
}

int OffsetRequestBlock::decode(packetDecoder &pd, int16_t version)
{
    m_leader_epoch = -1;
    int err = 0;

    if (version >= 4)
    {
        if ((err = pd.getInt32(m_leader_epoch)) != 0)
        {
            return err;
        }
    }

    if ((err = pd.getInt64(m_timestamp)) != 0)
    {
        return err;
    }

    if (version == 0)
    {
        if ((err = pd.getInt32(m_max_num_offsets)) != 0)
        {
            return err;
        }
    }

    return 0;
}

void OffsetRequest::set_version(int16_t v)
{
    m_version = v;
}

int OffsetRequest::encode(packetEncoder &pe)
{
    if (m_is_replica_id_set)
    {
        pe.putInt32(m_replica_id);
    }
    else
    {
        pe.putInt32(-1);
    }

    if (m_version >= 2)
    {
        pe.putBool(m_level == ReadCommitted);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    if (err != 0)
    {
        return err;
    }

    for (auto &[topic, partitions] : m_blocks)
    {
        err = pe.putString(topic);
        if (err != 0)
        {
            return err;
        }

        err = pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        if (err != 0)
        {
            return err;
        }

        for (auto &[partition, block] : partitions)
        {

            pe.putInt32(partition);
            if ((err = block.encode(pe, m_version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int OffsetRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    int32_t replicaID_val;
    int err = pd.getInt32(replicaID_val);
    if (err != 0)
    {
        return err;
    }
    if (replicaID_val >= 0)
    {
        set_replica_id(replicaID_val);
    }

    if (m_version >= 2)
    {
        bool tmp;
        if ((err = pd.getBool(tmp)) != 0)
        {
            return err;
        }
        m_level = tmp ? ReadCommitted : ReadUncommitted;
    }

    int32_t block_count;
    if ((err = pd.getArrayLength(block_count)) != 0)
    {
        return err;
    }
    if (block_count == 0)
    {
        return 0;
    }

    m_blocks.clear();
    for (int i = 0; i < block_count; ++i)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }

        int32_t partition_count;
        if ((err = pd.getArrayLength(partition_count)) != 0)
        {
            return err;
        }

        auto &partition_map = m_blocks[topic];
        for (int j = 0; j < partition_count; ++j)
        {
            int32_t partition;
            if ((err = pd.getInt32(partition)) != 0)
            {
                return err;
            }

            auto &block = partition_map[partition];
            if ((err = block.decode(pd, version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int16_t OffsetRequest::key() const
{
    return apiKeyListOffsets;
}

int16_t OffsetRequest::version() const
{
    return m_version;
}

int16_t OffsetRequest::header_version() const
{
    return 1;
}

bool OffsetRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

KafkaVersion OffsetRequest::required_version() const
{
    switch (m_version)
    {
    case 5:
        return V2_2_0_0;
    case 4:
        return V2_1_0_0;
    case 3:
        return V2_0_0_0;
    case 2:
        return V0_11_0_0;
    case 1:
        return V0_10_1_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_0_0_0;
    }
}

void OffsetRequest::set_replica_id(int32_t id)
{
    m_replica_id = id;
    m_is_replica_id_set = true;
}

int32_t OffsetRequest::replica_id()
{
    if (m_is_replica_id_set)
    {
        return m_replica_id;
    }
    return -1;
}

void OffsetRequest::add_block(const std::string &topic, int32_t partition_id, int64_t timestamp, int32_t max_offsets)
{
    auto &block = m_blocks[topic][partition_id];
    block.m_leader_epoch = -1;
    block.m_timestamp = timestamp;
    if (m_version == 0)
    {
        block.m_max_num_offsets = max_offsets;
    }
}

OffsetRequest::OffsetRequest(const KafkaVersion &version)
{
    if (version.IsAtLeast(V2_2_0_0))
    {
        m_version = 5;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        m_version = 4;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        m_version = 3;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        m_version = 2;
    }
    else if (version.IsAtLeast(V0_10_1_0))
    {
        m_version = 1;
    }
    else
    {
        m_version = 0;
    }
}