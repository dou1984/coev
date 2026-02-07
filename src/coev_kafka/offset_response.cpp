#include "version.h"
#include "offset_response.h"
#include <stdexcept>

OffsetResponseBlock::OffsetResponseBlock()
    : m_err(static_cast<KError>(0)), m_timestamp(0), m_offset(0), m_leader_epoch(-1) {}

int OffsetResponseBlock::decode(packet_decoder &pd, int16_t version)
{
    int err = pd.getKError(m_err);
    if (err != 0)
    {
        return err;
    }

    if (version == 0)
    {
        return pd.getInt64Array(m_offsets);
    }

    if (version >= 1)
    {
        if ((err = pd.getInt64(m_timestamp)) != 0)
        {
            return err;
        }
        if ((err = pd.getInt64(m_offset)) != 0)
        {
            return err;
        }
        m_offsets = {m_offset};
    }

    if (version >= 4)
    {
        if ((err = pd.getInt32(m_leader_epoch)) != 0)
        {
            return err;
        }
    }

    return 0;
}

int OffsetResponseBlock::encode(packet_encoder &pe, int16_t version) const
{
    pe.putKError(m_err);

    if (version == 0)
    {
        return pe.putInt64Array(m_offsets);
    }

    if (version >= 1)
    {
        pe.putInt64(m_timestamp);
        pe.putInt64(m_offset);
    }

    if (version >= 4)
    {
        pe.putInt32(m_leader_epoch);
    }

    return 0;
}

void OffsetResponse::set_version(int16_t v)
{
    m_version = v;
}

int OffsetResponse::decode(packet_decoder &pd, int16_t version)
{
    if (version >= 2)
    {
        int err = pd.getDurationMs(m_throttle_time);
        if (err != 0)
        {
            return err;
        }
    }

    int32_t num_topics;
    int err = pd.getArrayLength(num_topics);
    if (err != 0)
    {
        return err;
    }

    m_blocks.clear();
    m_blocks.reserve(num_topics);
    for (int i = 0; i < num_topics; ++i)
    {
        std::string name;
        if ((err = pd.getString(name)) != 0)
        {
            return err;
        }

        int32_t num_blocks;
        if ((err = pd.getArrayLength(num_blocks)) != 0)
        {
            return err;
        }

        auto &partitions = m_blocks[name];
        for (int j = 0; j < num_blocks; ++j)
        {
            int32_t id;
            if ((err = pd.getInt32(id)) != 0)
            {
                return err;
            }

            if ((err = partitions[id].decode(pd, version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

OffsetResponseBlock &OffsetResponse::get_block(const std::string &topic, int32_t partition)
{
    auto tit = m_blocks.find(topic);
    if (tit != m_blocks.end())
    {
        auto pit = tit->second.find(partition);
        if (pit != tit->second.end())
        {
            return pit->second;
        }
    }
    static OffsetResponseBlock _;
    return _;
}
bool OffsetResponse::has_block(const std::string &topic, int32_t partition)
{
    auto tit = m_blocks.find(topic);
    if (tit != m_blocks.end())
    {
        auto pit = tit->second.find(partition);
        if (pit != tit->second.end())
        {
            return true;
        }
    }
    return false;
}
int OffsetResponse::encode(packet_encoder &pe) const
{
    if (m_version >= 2)
    {
        pe.putDurationMs(m_throttle_time);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    if (err != 0)
    {
        return err;
    }

    for (auto &[topic, partitions] : m_blocks)
    {

        if ((err = pe.putString(topic)) != 0)
        {
            return err;
        }

        if ((err = pe.putArrayLength(static_cast<int32_t>(partitions.size()))) != 0)
        {
            return err;
        }

        for (auto &[partition, block] : partitions)
        {
            pe.putInt32(partition);
            if ((err = block.encode(pe, version())) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int16_t OffsetResponse::key() const
{
    return apiKeyListOffsets;
}

int16_t OffsetResponse::version() const
{
    return m_version;
}

int16_t OffsetResponse::header_version() const
{
    return 0;
}

bool OffsetResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

KafkaVersion OffsetResponse::required_version() const
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

std::chrono::milliseconds OffsetResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}

void OffsetResponse::add_topic_partition(const std::string &topic, int32_t partition, int64_t offset)
{

    auto &block = m_blocks[topic][partition];
    block.m_offsets = {offset};
    block.m_offset = offset;
}