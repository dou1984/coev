#include <iostream>
#include "version.h"
#include "api_versions.h"
#include "offset_commit_request.h"

int OffsetCommitRequestBlock::encode(PEncoder &pe, int16_t version)
{
    pe.putInt64(Offset);

    if (version == 1)
    {
        pe.putInt64(Timestamp);
    }
    else if (Timestamp != 0)
    {
    }
    if (version >= 6)
    {
        pe.putInt32(CommittedLeaderEpoch);
    }
    return pe.putString(Metadata);
}

int OffsetCommitRequestBlock::decode(PDecoder &pd, int16_t version)
{
    auto err = pd.getInt64(Offset);
    if (err != 0)
        return err;

    if (version == 1)
    {
        err = pd.getInt64(Timestamp);
        if (err != 0)
            return err;
    }
    if (version >= 6)
    {
        err = pd.getInt32(CommittedLeaderEpoch);
        if (err != 0)
            return err;
    }
    err = pd.getString(Metadata);
    return err;
}

OffsetCommitRequest::OffsetCommitRequest()
    : ConsumerGroupGeneration(0), RetentionTime(0), Version(0)
{
}

void OffsetCommitRequest::setVersion(int16_t v)
{
    Version = v;
}

int OffsetCommitRequest::encode(PEncoder &pe)
{
    if (Version < 0 || Version > 7)
    {
        return ErrConsumerOffsetNotAdvanced;
    }

    auto err = pe.putString(ConsumerGroup);

    if (Version >= 1)
    {
        pe.putInt32(ConsumerGroupGeneration);
        pe.putString(ConsumerID);
    }
    else
    {
        if (ConsumerGroupGeneration != 0)
        {
        }
        if (!ConsumerID.empty())
        {
        }
    }

    if (Version >= 2 && Version <= 4)
    {
        pe.putInt64(RetentionTime);
    }
    else if (RetentionTime != 0)
    {
    }

    if (Version >= 7)
    {
        pe.putNullableString(GroupInstanceId);
    }

    pe.putArrayLength(static_cast<int32_t>(blocks.size()));
    for (auto &topicEntry : blocks)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;
        pe.putString(topic);
        pe.putArrayLength(static_cast<int32_t>(partitions.size()));
        for (auto &partitionEntry : partitions)
        {
            pe.putInt32(partitionEntry.first);
            partitionEntry.second->encode(pe, Version);
        }
    }
    return 0;
}

int OffsetCommitRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    auto err = pd.getString(ConsumerGroup);
    if (err != 0)
        return err;

    if (Version >= 1)
    {
        pd.getInt32(ConsumerGroupGeneration);
        pd.getString(ConsumerID);
    }

    if (Version >= 2 && Version <= 4)
    {
        pd.getInt64(RetentionTime);
    }

    if (Version >= 7)
    {
        pd.getNullableString(GroupInstanceId);
    }

    int32_t topicCount;
    err = pd.getArrayLength(topicCount);
    if (topicCount == 0)
    {
        return err;
    }

    blocks.clear();
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
        auto &partitionMap = blocks[topic];
        for (int j = 0; j < partitionCount; ++j)
        {
            int32_t partition;
            err = pd.getInt32(partition);
            if (err != 0)
            {
                return err;
            }
            auto block = std::make_shared<OffsetCommitRequestBlock>();
            block->decode(pd, Version);
            partitionMap[partition] = block;
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
    return Version;
}

int16_t OffsetCommitRequest::headerVersion() const
{
    return 1;
}

bool OffsetCommitRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 7;
}

KafkaVersion OffsetCommitRequest::requiredVersion() const
{
    switch (Version)
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

void OffsetCommitRequest::AddBlock(const std::string &topic, int32_t partitionID, int64_t offset, int64_t timestamp, const std::string &metadata)
{
    AddBlockWithLeaderEpoch(topic, partitionID, offset, 0, timestamp, metadata);
}

void OffsetCommitRequest::AddBlockWithLeaderEpoch(const std::string &topic, int32_t partitionID, int64_t offset, int32_t leaderEpoch, int64_t timestamp, const std::string &metadata)
{
    if (blocks.find(topic) == blocks.end())
    {
        blocks[topic] = std::unordered_map<int32_t, std::shared_ptr<OffsetCommitRequestBlock>>();
    }
    blocks[topic][partitionID] = std::make_shared<OffsetCommitRequestBlock>(offset, timestamp, leaderEpoch, metadata);
}

std::pair<int64_t, std::string> OffsetCommitRequest::Offset(const std::string &topic, int32_t partitionID) const
{
    auto topicIt = blocks.find(topic);
    if (topicIt == blocks.end())
    {
        throw std::runtime_error("no such offset");
    }
    auto &partitions = topicIt->second;
    auto partIt = partitions.find(partitionID);
    if (partIt == partitions.end())
    {
        throw std::runtime_error("no such offset");
    }
    auto &block = partIt->second;
    return std::make_pair(block->Offset, block->Metadata);
}