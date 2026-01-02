#include "version.h"
#include "offset_fetch_response.h"
#include "api_versions.h"

int OffsetFetchResponseBlock::encode(PEncoder &pe, int16_t version)
{
    pe.putInt64(Offset);

    if (version >= 5)
    {
        pe.putInt32(LeaderEpoch);
    }

    pe.putString(Metadata);
    pe.putKError(Err);
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int OffsetFetchResponseBlock::decode(PDecoder &pd, int16_t version)
{
    auto err = pd.getInt64(Offset);
    if (err)
    {
        return err;
    }
    if (version >= 5)
    {
        err = pd.getInt32(LeaderEpoch);
        if (err)
        {
            return err;
        }
    }
    else
    {
        LeaderEpoch = -1;
    }

    err = pd.getString(Metadata);
    if (err)
    {
        return err;
    }
    err = pd.getKError(Err);
    if (err)
    {
        return err;
    }

    int32_t _;
    pd.getEmptyTaggedFieldArray(_);
    return 0;
}

OffsetFetchResponse::OffsetFetchResponse()
    : Version(0), Err(ErrNoError)
{
}

void OffsetFetchResponse::setVersion(int16_t v)
{
    Version = v;
}

int OffsetFetchResponse::encode(PEncoder &pe)
{
    if (Version >= 3)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putArrayLength(static_cast<int32_t>(Blocks.size()));
    for (auto &topicEntry : Blocks)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partEntry : partitions)
        {
            pe.putInt32(partEntry.first);
            partEntry.second->encode(pe, Version);
        }
        pe.putEmptyTaggedFieldArray();
    }

    if (Version >= 2)
    {
        pe.putKError(Err);
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int OffsetFetchResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (version >= 3)
    {
        auto err = pd.getDurationMs(ThrottleTime);
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
        Blocks.clear();
        Blocks.reserve(numTopics);
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

            auto &partitionMap = Blocks[name];
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
        auto err = pd.getKError(Err);
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
    return Version;
}

int16_t OffsetFetchResponse::headerVersion() const
{
    return Version >= 6 ? 1 : 0;
}

bool OffsetFetchResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 7;
}

bool OffsetFetchResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool OffsetFetchResponse::isFlexibleVersion(int16_t version)
{
    return version >= 6;
}

KafkaVersion OffsetFetchResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds OffsetFetchResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}

std::shared_ptr<OffsetFetchResponseBlock> OffsetFetchResponse::GetBlock(const std::string &topic, int32_t partition) const
{
    auto topicIt = Blocks.find(topic);
    if (topicIt == Blocks.end())
        return nullptr;
    auto partIt = topicIt->second.find(partition);
    if (partIt == topicIt->second.end())
        return nullptr;
    return partIt->second;
}

void OffsetFetchResponse::AddBlock(const std::string &topic, int32_t partition, std::shared_ptr<OffsetFetchResponseBlock> block)
{
    Blocks[topic][partition] = block;
}