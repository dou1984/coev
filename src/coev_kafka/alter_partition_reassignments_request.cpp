#include "version.h"
#include "api_versions.h"
#include "alter_partition_reassignments_request.h"
int AlterPartitionReassignmentsBlock::encode(PEncoder &pe)
{
    if (pe.putNullableInt32Array(m_replicas) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterPartitionReassignmentsBlock::decode(PDecoder &pd)
{
    if (pd.getInt32Array(m_replicas) != ErrNoError)
    {
        return ErrEncodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void AlterPartitionReassignmentsRequest::set_version(int16_t v)
{
    m_version = v;
}

int AlterPartitionReassignmentsRequest::encode(PEncoder &pe)
{
    pe.putInt32(m_timeout.count());

    if (pe.putArrayLength(static_cast<int32_t>(m_blocks.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &topicPair : m_blocks)
    {
        auto &topic = topicPair.first;
        auto &partitions = topicPair.second;

        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (pe.putArrayLength(static_cast<int32_t>(partitions.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }

        for (auto &partitionPair : partitions)
        {
            pe.putInt32(partitionPair.first);
            if (partitionPair.second->encode(pe) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterPartitionReassignmentsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    int32_t timeout;
    if (pd.getInt32(timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_timeout = std::chrono::milliseconds(timeout);

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

            auto &partitionMap = m_blocks[topic];
            for (int32_t j = 0; j < partitionCount; ++j)
            {
                int32_t partition;
                if (pd.getInt32(partition) != ErrNoError)
                {
                    return ErrDecodeError;
                }

                auto block = std::make_shared<AlterPartitionReassignmentsBlock>();
            if (block->decode(pd) != ErrNoError)
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
    return m_version;
}

int16_t AlterPartitionReassignmentsRequest::header_version() const
{
    return 2;
}

bool AlterPartitionReassignmentsRequest::is_valid_version() const
{
    return m_version == 0;
}

bool AlterPartitionReassignmentsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool AlterPartitionReassignmentsRequest::is_flexible_version(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterPartitionReassignmentsRequest::required_version() const
{
    return V2_4_0_0;
}
std::chrono::milliseconds AlterPartitionReassignmentsRequest::throttle_time() const
{
    return m_timeout;
}

void AlterPartitionReassignmentsRequest::AddBlock(const std::string &topic, int32_t partitionID, const std::vector<int32_t> &replicas)
{
    m_blocks[topic][partitionID] = std::make_shared<AlterPartitionReassignmentsBlock>(replicas);
}