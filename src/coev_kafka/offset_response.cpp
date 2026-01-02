#include "version.h"
#include "offset_response.h"
#include <stdexcept>

OffsetResponseBlock::OffsetResponseBlock()
    : Err(static_cast<KError>(0)), Timestamp(0), Offset(0), LeaderEpoch(-1) {}

int OffsetResponseBlock::decode(PDecoder &pd, int16_t version)
{
    int err = pd.getKError(Err);
    if (err != 0)
    {
        return err;
    }

    if (version == 0)
    {
        return pd.getInt64Array(Offsets);
    }

    if (version >= 1)
    {
        if ((err = pd.getInt64(Timestamp)) != 0)
        {
            return err;
        }
        if ((err = pd.getInt64(Offset)) != 0)
        {
            return err;
        }
        Offsets = {Offset};
    }

    if (version >= 4)
    {
        if ((err = pd.getInt32(LeaderEpoch)) != 0)
        {
            return err;
        }
    }

    return 0;
}

int OffsetResponseBlock::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(Err);

    if (version == 0)
    {
        return pe.putInt64Array(Offsets);
    }

    if (version >= 1)
    {
        pe.putInt64(Timestamp);
        pe.putInt64(Offset);
    }

    if (version >= 4)
    {
        pe.putInt32(LeaderEpoch);
    }

    return 0;
}

void OffsetResponse::setVersion(int16_t v)
{
    Version = v;
}

int OffsetResponse::decode(PDecoder &pd, int16_t version)
{
    if (version >= 2)
    {
        int err = pd.getDurationMs(ThrottleTime);
        if (err != 0)
        {
            return err;
        }
    }

    int32_t numTopics;
    int err = pd.getArrayLength(numTopics);
    if (err != 0)
    {
        return err;
    }

    Blocks_.clear();
    Blocks_.reserve(numTopics);
    for (int i = 0; i < numTopics; ++i)
    {
        std::string name;
        if ((err = pd.getString(name)) != 0)
        {
            return err;
        }

        int32_t numBlocks;
        if ((err = pd.getArrayLength(numBlocks)) != 0)
        {
            return err;
        }

        auto &partitionMap = Blocks_[name];
        partitionMap.reserve(numBlocks);
        for (int j = 0; j < numBlocks; ++j)
        {
            int32_t id;
            if ((err = pd.getInt32(id)) != 0)
            {
                return err;
            }

            auto block = std::make_shared<OffsetResponseBlock>();
            if ((err = block->decode(pd, version)) != 0)
            {
                return err;
            }
            partitionMap[id] = block;
        }
    }

    return 0;
}

std::shared_ptr<OffsetResponseBlock> OffsetResponse::GetBlock(const std::string &topic, int32_t partition)
{
    auto topicIt = Blocks_.find(topic);
    if (topicIt == Blocks_.end())
    {
        return nullptr;
    }
    auto partitionIt = topicIt->second.find(partition);
    if (partitionIt == topicIt->second.end())
    {
        return nullptr;
    }
    return partitionIt->second;
}

int OffsetResponse::encode(PEncoder &pe)
{
    if (Version >= 2)
    {
        pe.putDurationMs(ThrottleTime);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(Blocks_.size()));
    if (err != 0)
    {
        return err;
    }

    for (auto &topic_pair : Blocks_)
    {
        const std::string &topic = topic_pair.first;
        auto &partitions = topic_pair.second;

        if ((err = pe.putString(topic)) != 0)
        {
            return err;
        }

        if ((err = pe.putArrayLength(static_cast<int32_t>(partitions.size()))) != 0)
        {
            return err;
        }

        for (auto &partition_pair : partitions)
        {
            int32_t partition = partition_pair.first;
            auto &block = partition_pair.second;

            pe.putInt32(partition);
            if ((err = block->encode(pe, version())) != 0)
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
    return Version;
}

int16_t OffsetResponse::headerVersion() const
{
    return 0;
}

bool OffsetResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

KafkaVersion OffsetResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds OffsetResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}

void OffsetResponse::AddTopicPartition(const std::string &topic, int32_t partition, int64_t offset)
{
    if (Blocks_.find(topic) == Blocks_.end())
    {
        Blocks_[topic] = std::unordered_map<int32_t, std::shared_ptr<OffsetResponseBlock>>();
    }
    auto block = std::make_shared<OffsetResponseBlock>();
    block->Offsets = {offset};
    block->Offset = offset;
    Blocks_[topic][partition] = block;
}