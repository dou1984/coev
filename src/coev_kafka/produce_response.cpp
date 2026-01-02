#include "version.h"
#include "produce_response.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include <limits>
#include <stdexcept>

int ProduceResponseBlock::decode(PDecoder &pd, int16_t version)
{
    if (int err = pd.getKError(Err); err != 0)
        return err;
    if (int err = pd.getInt64(Offset); err != 0)
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
            Timestamp = std::chrono::system_clock::from_time_t(seconds) + std::chrono::nanoseconds(nanos);
        }
    }

    if (version >= 5)
    {
        if (int err = pd.getInt64(StartOffset); err != 0)
            return err;
    }

    return 0;
}

int ProduceResponseBlock::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(Err);
    pe.putInt64(Offset);

    if (version >= 2)
    {
        int64_t timestamp = -1;
        if (Timestamp.time_since_epoch().count() > 0)
        {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Timestamp.time_since_epoch()).count();
            timestamp = ms;
        }
        else if (Timestamp != std::chrono::system_clock::time_point{})
        {
            return -1;
        }
        pe.putInt64(timestamp);
    }

    if (version >= 5)
    {
        pe.putInt64(StartOffset);
    }

    return 0;
}

void ProduceResponse::setVersion(int16_t v)
{
    Version = v;
}

int ProduceResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int32_t numTopics;
    if (int err = pd.getArrayLength(numTopics); err != 0)
        return err;

    Blocks.clear();
    Blocks.reserve(numTopics);
    for (int i = 0; i < numTopics; ++i)
    {
        std::string name;
        if (int err = pd.getString(name); err != 0)
            return err;

        int32_t numBlocks;
        if (int err = pd.getArrayLength(numBlocks); err != 0)
            return err;

        auto &partitionMap = Blocks[name];
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

    if (Version >= 1)
    {
        if (int err = pd.getDurationMs(ThrottleTime); err != 0)
            return err;
    }

    return 0;
}

int ProduceResponse::encode(PEncoder &pe)
{
    if (int err = pe.putArrayLength(static_cast<int32_t>(Blocks.size())); err != 0)
        return err;

    for (auto &[topic, partitions] : Blocks)
    {
        if (int err = pe.putString(topic); err != 0)
            return err;
        if (int err = pe.putArrayLength(static_cast<int32_t>(partitions.size())); err != 0)
            return err;

        for (auto &[id, prb] : partitions)
        {
            pe.putInt32(id);
            if (int err = prb->encode(pe, Version); err != 0)
                return err;
        }
    }

    if (Version >= 1)
    {
         pe.putDurationMs(ThrottleTime);
    }

    return 0;
}

int16_t ProduceResponse::key() const
{
    return 0;
}

int16_t ProduceResponse::version() const
{
    return Version;
}

int16_t ProduceResponse::headerVersion() const
{
    return 0;
}

bool ProduceResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 7;
}

KafkaVersion ProduceResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds ProduceResponse::throttleTime() const
{
    return ThrottleTime;
}

std::shared_ptr<ProduceResponseBlock> ProduceResponse::GetBlock(const std::string &topic, int32_t partition) const
{
    auto topicIt = Blocks.find(topic);
    if (topicIt == Blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void ProduceResponse::AddTopicPartition(const std::string &topic, int32_t partition, KError err)
{
    auto &byTopic = Blocks[topic];
    auto block = std::make_shared<ProduceResponseBlock>();
    block->Err = err;
    if (Version >= 2)
    {
        block->Timestamp = std::chrono::system_clock::now();
    }
    byTopic[partition] = block;
}