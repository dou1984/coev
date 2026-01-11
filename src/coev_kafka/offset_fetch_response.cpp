#include "version.h"
#include "offset_fetch_response.h"
#include "api_versions.h"

int OffsetFetchResponseBlock::encode(PEncoder &pe, int16_t version)
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

int OffsetFetchResponseBlock::decode(PDecoder &pd, int16_t version)
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

int OffsetFetchResponse::encode(PEncoder &pe)
{
    if (m_version >= 3)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putArrayLength(static_cast<int32_t>(m_blocks.size()));
    for (auto &topicEntry : m_blocks)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partEntry : partitions)
        {
            pe.putInt32(partEntry.first);
            partEntry.second->encode(pe, m_version);
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

int OffsetFetchResponse::decode(PDecoder &pd, int16_t version)
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

    int32_t numTopics;
    auto err = pd.getArrayLength(numTopics);
    if (err)
    {
        return err;
    }
    if (numTopics > 0)
    {
        m_blocks.clear();
        m_blocks.reserve(numTopics);
        for (int i = 0; i < numTopics; ++i)
        {
            std::string name;
            err = pd.getString(name);
            if (err)
            {
                return err;
            }
            int32_t numBlocks;
            err = pd.getArrayLength(numBlocks);
            if (err)
            {
                return err;
            }

            auto &partitionMap = m_blocks[name];
            if (numBlocks > 0)
            {
                partitionMap.reserve(numBlocks);
                for (int j = 0; j < numBlocks; ++j)
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

bool OffsetFetchResponse::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool OffsetFetchResponse::isFlexibleVersion(int16_t version)
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

std::shared_ptr<OffsetFetchResponseBlock> OffsetFetchResponse::GetBlock(const std::string &topic, int32_t partition) const
{
    auto topicIt = m_blocks.find(topic);
    if (topicIt == m_blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void OffsetFetchResponse::AddBlock(const std::string &topic, int32_t partition, std::shared_ptr<OffsetFetchResponseBlock> block)
{
    m_blocks[topic][partition] = block;
}