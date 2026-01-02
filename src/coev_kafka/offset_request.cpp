#include "version.h"
#include "offset_request.h"
#include <stdexcept>

OffsetRequestBlock::OffsetRequestBlock()
    : currentLeaderEpoch(-1), timestamp(0), maxNumOffsets(0) {}

int OffsetRequestBlock::encode(PEncoder &pe, int16_t version)
{
    if (version >= 4)
    {
        pe.putInt32(currentLeaderEpoch);
    }

    pe.putInt64(timestamp);

    if (version == 0)
    {
        pe.putInt32(maxNumOffsets);
    }

    return 0;
}

int OffsetRequestBlock::decode(PDecoder &pd, int16_t version)
{
    currentLeaderEpoch = -1;
    int err = 0;

    if (version >= 4)
    {
        if ((err = pd.getInt32(currentLeaderEpoch)) != 0)
        {
            return err;
        }
    }

    if ((err = pd.getInt64(timestamp)) != 0)
    {
        return err;
    }

    if (version == 0)
    {
        if ((err = pd.getInt32(maxNumOffsets)) != 0)
        {
            return err;
        }
    }

    return 0;
}

void OffsetRequest::setVersion(int16_t v)
{
    Version = v;
}

int OffsetRequest::encode(PEncoder &pe)
{
    if (isReplicaIDSet)
    {
        pe.putInt32(replicaID);
    }
    else
    {
        pe.putInt32(-1);
    }

    if (Version >= 2)
    {
        pe.putBool(Level == ReadCommitted);
    }

    int err = pe.putArrayLength(static_cast<int32_t>(blocks.size()));
    if (err != 0)
    {
        return err;
    }

    for (auto &topic_pair : blocks)
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
            if ((err = block->encode(pe, Version)) != 0)
            {
                return err;
            }
        }
    }

    return 0;
}

int OffsetRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
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

    if (Version >= 2)
    {
        bool tmp;
        if ((err = pd.getBool(tmp)) != 0)
        {
            return err;
        }
        Level = tmp ? ReadCommitted : ReadUncommitted;
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

    blocks.clear();
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

        auto &partitionMap = blocks[topic];
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
    return Version;
}

int16_t OffsetRequest::headerVersion() const
{
    return 1;
}

bool OffsetRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 5;
}

KafkaVersion OffsetRequest::requiredVersion() const
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

void OffsetRequest::SetReplicaID(int32_t id)
{
    replicaID = id;
    isReplicaIDSet = true;
}

int32_t OffsetRequest::ReplicaID()
{
    if (isReplicaIDSet)
    {
        return replicaID;
    }
    return -1;
}

void OffsetRequest::AddBlock(const std::string &topic, int32_t partitionID, int64_t timestamp, int32_t maxOffsets)
{
    if (blocks.find(topic) == blocks.end())
    {
        blocks[topic] = std::unordered_map<int32_t, std::shared_ptr<OffsetRequestBlock>>();
    }

    auto block = std::make_shared<OffsetRequestBlock>();
    block->currentLeaderEpoch = -1;
    block->timestamp = timestamp;
    if (Version == 0)
    {
        block->maxNumOffsets = maxOffsets;
    }

    blocks[topic][partitionID] = block;
}

std::shared_ptr<OffsetRequest> NewOffsetRequest(const KafkaVersion &version)
{
    auto request = std::make_shared<OffsetRequest>();
    if (version.IsAtLeast(V2_2_0_0))
    {
        request->Version = 5;
    }
    else if (version.IsAtLeast(V2_1_0_0))
    {
        request->Version = 4;
    }
    else if (version.IsAtLeast(V2_0_0_0))
    {
        request->Version = 3;
    }
    else if (version.IsAtLeast(V0_11_0_0))
    {
        request->Version = 2;
    }
    else if (version.IsAtLeast(V0_10_1_0))
    {
        request->Version = 1;
    }
    else
    {
        request->Version = 0;
    }
    return request;
}