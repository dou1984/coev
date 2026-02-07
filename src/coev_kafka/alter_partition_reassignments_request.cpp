#include "version.h"
#include "api_versions.h"
#include "alter_partition_reassignments_request.h"
int AlterPartitionReassignmentsBlock::encode(packet_encoder &pe) const
{
    if (pe.putNullableInt32Array(m_replicas) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterPartitionReassignmentsBlock::decode(packet_decoder &pd)
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

int AlterPartitionReassignmentsRequest::encode(packet_encoder &pe) const
{
    pe.putInt32(m_timeout.count());

    if (pe.putArrayLength(static_cast<int32_t>(m_blocks.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &[topic, partitions] : m_blocks)
    {

        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (pe.putArrayLength(static_cast<int32_t>(partitions.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }

        for (auto &[partition, block] : partitions)
        {
            pe.putInt32(partition);
            if (block.encode(pe) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterPartitionReassignmentsRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    int32_t timeout;
    if (pd.getInt32(timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_timeout = std::chrono::milliseconds(timeout);

    int32_t topic_count;
    if (pd.getArrayLength(topic_count) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (topic_count > 0)
    {
        for (int32_t i = 0; i < topic_count; ++i)
        {
            std::string topic;
            if (pd.getString(topic) != ErrNoError)
            {
                return ErrDecodeError;
            }

            int32_t partition_count;
            if (pd.getArrayLength(partition_count) != ErrNoError)
            {
                return ErrDecodeError;
            }

            auto &partitions = m_blocks[topic];
            for (int32_t j = 0; j < partition_count; ++j)
            {
                int32_t partition;
                if (pd.getInt32(partition) != ErrNoError)
                {
                    return ErrDecodeError;
                }

                if (partitions[partition].decode(pd) != ErrNoError)
                {
                    return ErrDecodeError;
                }
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

void AlterPartitionReassignmentsRequest::add_block(const std::string &topic, int32_t partitionID, const std::vector<int32_t> &replicas)
{
    m_blocks[topic][partitionID] = replicas;
}