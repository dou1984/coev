#include "version.h"
#include "offset_fetch_response.h"
#include "api_versions.h"

int OffsetFetchResponseBlock::encode(packet_encoder &pe, int16_t version) const
{
    pe.putInt64(m_offset);

    if (version >= 5)
    {
        pe.putInt32(m_leader_epoch);
    }

    pe.putString(m_metadata);
    pe.putKError(m_err);
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int OffsetFetchResponseBlock::decode(packet_decoder &pd, int16_t version)
{
    auto err = pd.getInt64(m_offset);
    if (err)
    {
        return err;
    }
    if (version >= 5)
    {
        err = pd.getInt32(m_leader_epoch);
        if (err)
        {
            return err;
        }
    }
    else
    {
        m_leader_epoch = -1;
    }

    err = pd.getString(m_metadata);
    if (err)
    {
        return err;
    }
    err = pd.getKError(m_err);
    if (err)
    {
        return err;
    }

    int32_t _;
    pd.getEmptyTaggedFieldArray(_);
    return 0;
}

OffsetFetchResponse::OffsetFetchResponse()
    : m_version(0), m_err(ErrNoError)
{
}

void OffsetFetchResponse::set_version(int16_t v)
{
    m_version = v;
}

int OffsetFetchResponse::encode(packet_encoder &pe) const
{
    if (m_version >= 3)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    for (const auto &[topic, partitions] : m_blocks)
    {

        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (const auto &[pid, partition] : partitions)
        {
            pe.putInt32(pid);
            partition->encode(pe, m_version);
        }
        pe.putEmptyTaggedFieldArray();
    }

    if (m_version >= 2)
    {
        pe.putKError(m_err);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int OffsetFetchResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (version >= 3)
    {
        auto err = pd.getDurationMs(m_throttle_time);
        if (err)
        {
            return err;
        }
    }

    int32_t num_topics;
    auto err = pd.getArrayLength(num_topics);
    if (err)
    {
        return err;
    }
    if (num_topics > 0)
    {
        m_blocks.clear();
        m_blocks.reserve(num_topics);
        for (int i = 0; i < num_topics; ++i)
        {
            std::string name;
            err = pd.getString(name);
            if (err)
            {
                return err;
            }
            int32_t num_blocks;
            err = pd.getArrayLength(num_blocks);
            if (err)
            {
                return err;
            }

            auto &partitionMap = m_blocks[name];
            if (num_blocks > 0)
            {
                // std::map doesn't support reserve
                for (int j = 0; j < num_blocks; ++j)
                {
                    int32_t id;
                    err = pd.getInt32(id);
                    if (err)
                    {
                        return err;
                    }
                    auto block = std::make_shared<OffsetFetchResponseBlock>();
                    err = block->decode(pd, version);
                    if (err)
                    {
                        return err;
                    }
                    partitionMap[id] = block;
                }
            }
            int32_t _;
            pd.getEmptyTaggedFieldArray(_); // consume topic-level tagged fields
        }
    }

    if (version >= 2)
    {
        auto err = pd.getKError(m_err);
        if (err)
        {
            return err;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t OffsetFetchResponse::key() const
{
    return apiKeyOffsetFetch;
}

int16_t OffsetFetchResponse::version() const
{
    return m_version;
}

int16_t OffsetFetchResponse::header_version() const
{
    return m_version >= 6 ? 1 : 0;
}

bool OffsetFetchResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 7;
}

bool OffsetFetchResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool OffsetFetchResponse::is_flexible_version(int16_t version) const
{
    return version >= 6;
}

KafkaVersion OffsetFetchResponse::required_version() const
{
    switch (m_version)
    {
    case 7:
        return V2_5_0_0;
    case 6:
        return V2_4_0_0;
    case 5:
        return V2_1_0_0;
    case 4:
        return V2_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_10_2_0;
    case 1:
    case 0:
        return V0_8_2_0;
    default:
        return V2_5_0_0;
    }
}

std::chrono::milliseconds OffsetFetchResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}

std::shared_ptr<OffsetFetchResponseBlock> OffsetFetchResponse::get_block(const std::string &topic, int32_t partition) const
{
    auto topicIt = m_blocks.find(topic);
    if (topicIt == m_blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void OffsetFetchResponse::add_block(const std::string &topic, int32_t partition, std::shared_ptr<OffsetFetchResponseBlock> block)
{
    m_blocks[topic][partition] = block;
}