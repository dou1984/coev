#include <limits>
#include <stdexcept>
#include <coev/coev.h>
#include "version.h"
#include "produce_response.h"
#include "packet_decoder.h"
#include "packet_encoder.h"

int ProduceResponseBlock::decode(packetDecoder &pd, int16_t version)
{
    if (int err = pd.getKError(m_err); err != 0)
        return err;
    if (int err = pd.getInt64(m_offset); err != 0)
        return err;

    if (version >= 2)
    {
        int64_t millis;
        if (int err = pd.getInt64(millis); err != 0)
            return err;
        if (millis != -1)
        {
            auto seconds = millis / 1000;
            auto nanos = (millis % 1000) * 1000000LL;
            m_timestamp = std::chrono::system_clock::from_time_t(seconds) + std::chrono::nanoseconds(nanos);
        }
    }

    if (version >= 5)
    {
        if (int err = pd.getInt64(m_start_offset); err != 0)
            return err;
    }

    return 0;
}

int ProduceResponseBlock::encode(packetEncoder &pe, int16_t version)
{
    pe.putKError(m_err);
    pe.putInt64(m_offset);

    if (version >= 2)
    {
        int64_t timestamp = -1;
        if (m_timestamp.time_since_epoch().count() > 0)
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(m_timestamp.time_since_epoch()).count();
            timestamp = ms;
        }
        else if (m_timestamp != std::chrono::system_clock::time_point{})
        {
            return INVALID;
        }
        pe.putInt64(timestamp);
    }

    if (version >= 5)
    {
        pe.putInt64(m_start_offset);
    }

    return 0;
}

void ProduceResponse::set_version(int16_t v)
{
    m_version = v;
}

int ProduceResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int32_t numTopics;
    if (int err = pd.getArrayLength(numTopics); err != 0)
        return err;

    m_blocks.clear();
    m_blocks.reserve(numTopics);
    for (int i = 0; i < numTopics; ++i)
    {
        std::string name;
        if (int err = pd.getString(name); err != 0)
            return err;

        int32_t numBlocks;
        if (int err = pd.getArrayLength(numBlocks); err != 0)
            return err;

        auto &partitionMap = m_blocks[name];
        partitionMap.reserve(numBlocks);
        for (int j = 0; j < numBlocks; ++j)
        {
            int32_t id;
            if (int err = pd.getInt32(id); err != 0)
                return err;

            auto block = std::make_shared<ProduceResponseBlock>();
            if (int err = block->decode(pd, version); err != 0)
                return err;
            partitionMap[id] = block;
        }
    }

    if (m_version >= 1)
    {
        if (int err = pd.getDurationMs(m_throttle_time); err != 0)
            return err;
    }

    return 0;
}

int ProduceResponse::encode(packetEncoder &pe)
{
    if (int err = pe.putArrayLength(static_cast<int32_t>(m_blocks.size())); err != 0)
        return err;

    for (auto &[topic, partitions] : m_blocks)
    {
        if (int err = pe.putString(topic); err != 0)
            return err;
        if (int err = pe.putArrayLength(static_cast<int32_t>(partitions.size())); err != 0)
            return err;

        for (auto &[id, prb] : partitions)
        {
            pe.putInt32(id);
            if (int err = prb->encode(pe, m_version); err != 0)
                return err;
        }
    }

    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    return 0;
}

int16_t ProduceResponse::key() const
{
    return 0;
}

int16_t ProduceResponse::version() const
{
    return m_version;
}

int16_t ProduceResponse::header_version() const
{
    return 0;
}

bool ProduceResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 9;
}

KafkaVersion ProduceResponse::required_version() const
{
    switch (m_version)
    {
    case 7:
        return V2_1_0_0;
    case 6:
        return V2_0_0_0;
    case 5:
    case 4:
        return V1_0_0_0;
    case 3:
        return V0_11_0_0;
    case 2:
        return V0_10_0_0;
    case 1:
        return V0_9_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_1_0_0;
    }
}

std::chrono::milliseconds ProduceResponse::throttle_time() const
{
    return m_throttle_time;
}

std::shared_ptr<ProduceResponseBlock> ProduceResponse::GetBlock(const std::string &topic, int32_t partition) const
{
    auto topicIt = m_blocks.find(topic);
    if (topicIt == m_blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void ProduceResponse::AddTopicPartition(const std::string &topic, int32_t partition, KError err)
{
    auto &byTopic = m_blocks[topic];
    auto block = std::make_shared<ProduceResponseBlock>();
    block->m_err = err;
    if (m_version >= 2)
    {
        block->m_timestamp = std::chrono::system_clock::now();
    }
    byTopic[partition] = block;
}