#include "version.h"
#include "api_versions.h"
#include "alter_partition_reassignments_request.h"
int AlterPartitionReassignmentsBlock::encode(PEncoder &pe)
{
    if (!pe.putNullableInt32Array(replicas))
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return true;
}

int AlterPartitionReassignmentsBlock::decode(PDecoder &pd)
{
    if (pd.getInt32Array(replicas) != ErrNoError)
    {
        return ErrEncodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void AlterPartitionReassignmentsRequest::setVersion(int16_t v)
{
    Version = v;
}

int AlterPartitionReassignmentsRequest::encode(PEncoder &pe)
{
    pe.putInt32(Timeout.count());

    if (!pe.putArrayLength(static_cast<int32_t>(blocks.size())))
    {
        return ErrEncodeError;
    }

    for (auto &topicPair : blocks)
    {
        auto &topic = topicPair.first;
        auto &partitions = topicPair.second;

        if (!pe.putString(topic))
        {
            return ErrEncodeError;
        }

        if (!pe.putArrayLength(static_cast<int32_t>(partitions.size())))
        {
            return ErrEncodeError;
        }

        for (auto &partitionPair : partitions)
        {
            pe.putInt32(partitionPair.first);
            if (!partitionPair.second->encode(pe))
            {
                return ErrEncodeError;
            }
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int AlterPartitionReassignmentsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int32_t timeout;
    if (pd.getInt32(timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }
    Timeout = std::chrono::milliseconds(timeout);

    int32_t topicCount;
    if (pd.getArrayLength(topicCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (topicCount > 0)
    {
        for (int32_t i = 0; i < topicCount; ++i)
        {
            std::string topic;
            if (pd.getString(topic) != ErrNoError)
            {
                return ErrDecodeError;
            }

            int32_t partitionCount;
            if (pd.getArrayLength(partitionCount) != ErrNoError)
            {
                return ErrDecodeError;
            }

            auto &partitionMap = blocks[topic];
            for (int32_t j = 0; j < partitionCount; ++j)
            {
                int32_t partition;
                if (pd.getInt32(partition) != ErrNoError)
                {
                    return ErrDecodeError;
                }

                auto block = std::make_shared<AlterPartitionReassignmentsBlock>();
                if (!block->decode(pd))
                {
                    return ErrDecodeError;
                }
                partitionMap[partition] = block;
            }
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t AlterPartitionReassignmentsRequest::key() const
{
    return apiKeyAlterPartitionReassignments;
}

int16_t AlterPartitionReassignmentsRequest::version() const
{
    return Version;
}

int16_t AlterPartitionReassignmentsRequest::headerVersion() const
{
    return 2;
}

bool AlterPartitionReassignmentsRequest::isValidVersion() const
{
    return Version == 0;
}

bool AlterPartitionReassignmentsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool AlterPartitionReassignmentsRequest::isFlexibleVersion(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterPartitionReassignmentsRequest::requiredVersion() const
{
    return V2_4_0_0;
}
std::chrono::milliseconds AlterPartitionReassignmentsRequest::throttleTime() const
{
    return Timeout;
}

void AlterPartitionReassignmentsRequest::AddBlock(const std::string &topic, int32_t partitionID, const std::vector<int32_t> &replicas)
{
    blocks[topic][partitionID] = std::make_shared<AlterPartitionReassignmentsBlock>(replicas);
}