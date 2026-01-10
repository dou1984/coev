#include "version.h"
#include "offset_request.h"
#include <stdexcept>

OffsetRequestBlock::OffsetRequestBlock()
    : m_leader_epoch(-1), m_timestamp(0), m_max_num_offsets(0) {}

int OffsetRequestBlock::encode(PEncoder &pe, int16_t version)
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

int OffsetRequestBlock::decode(PDecoder &pd, int16_t version)
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

int OffsetRequest::encode(PEncoder &pe)
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

    for (auto &topic_pair : m_blocks)
    {
        const std::string &topic = topic_pair.first;
        auto &partitions = topic_pair.second;

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

        for (auto &partition_pair : partitions)
        {
            int32_t partition = partition_pair.first;
            auto &block = partition_pair.second;

            pe.putInt32(partition);
            if ((err = block->encode(pe, m_version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int OffsetRequest::decode(PDecoder &pd, int16_t version)
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
        SetReplicaID(replicaID_val);
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

    int32_t blockCount;
    if ((err = pd.getArrayLength(blockCount)) != 0)
    {
        return err;
    }
    if (blockCount == 0)
    {
        return 0;
    }

    m_blocks.clear();
    for (int i = 0; i < blockCount; ++i)
    {
        std::string topic;
        if ((err = pd.getString(topic)) != 0)
        {
            return err;
        }

        int32_t partitionCount;
        if ((err = pd.getArrayLength(partitionCount)) != 0)
        {
            return err;
        }

        auto &partitionMap = m_blocks[topic];
        for (int j = 0; j < partitionCount; ++j)
        {
            int32_t partition;
            if ((err = pd.getInt32(partition)) != 0)
            {
                return err;
            }

            auto block = std::make_shared<OffsetRequestBlock>();
            if ((err = block->decode(pd, version)) != 0)
            {
                return err;
            }
            partitionMap[partition] = block;
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

int16_t OffsetRequest::headerVersion() const
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

void OffsetRequest::SetReplicaID(int32_t id)
{
    m_replica_id = id;
    m_is_replica_id_set = true;
}

int32_t OffsetRequest::ReplicaID()
{
    if (m_is_replica_id_set)
    {
        return m_replica_id;
    }
    return -1;
}

void OffsetRequest::AddBlock(const std::string &topic, int32_t partitionID, int64_t timestamp, int32_t maxOffsets)
{
    if (m_blocks.find(topic) == m_blocks.end())
    {
        m_blocks[topic] = std::unordered_map<int32_t, std::shared_ptr<OffsetRequestBlock>>();
    }

    auto block = std::make_shared<OffsetRequestBlock>();
    block->m_leader_epoch = -1;
    block->m_timestamp = timestamp;
    if (m_version == 0)
    {
        block->m_max_num_offsets = maxOffsets;
    }

    m_blocks[topic][partitionID] = block;
}

std::shared_ptr<OffsetRequest> NewOffsetRequest(const KafkaVersion &version)
{
    auto request = std::make_shared<OffsetRequest>();
    if (version.IsAtLeast(V2_2_0_0))
    {
        request->m_version = 5;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        request->m_version = 4;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        request->m_version = 3;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        request->m_version = 2;
    }
    else if (version.IsAtLeast(V0_10_1_0))
    {
        request->m_version = 1;
    }
    else
    {
        request->m_version = 0;
    }
    return request;
}