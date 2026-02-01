#include "version.h"
#include "alter_partition_reassignments_response.h"
#include "api_versions.h"

int AlterPartitionReassignmentsErrorBlock::encode(packet_encoder &pe) const
{
    pe.putKError(m_error_code);
    if (pe.putNullableString(m_error_message) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterPartitionReassignmentsErrorBlock::decode(packet_decoder &pd)
{
    if (pd.getKError(m_error_code) != ErrNoError)
    {
        return ErrEncodeError;
    }
    if (pd.getNullableString(m_error_message) != ErrNoError)
    {
        return ErrEncodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void AlterPartitionReassignmentsResponse::set_version(int16_t v)
{
    m_version = v;
}

void AlterPartitionReassignmentsResponse::add_error(const std::string &topic, int32_t partition, KError kerror, std::string message)
{
    m_errors[topic][partition] = std::make_shared<AlterPartitionReassignmentsErrorBlock>(kerror, message);
}

int AlterPartitionReassignmentsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);
    pe.putKError(m_error_code);
    if (pe.putNullableString(m_error_message) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_errors.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &topicPair : m_errors)
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

int AlterPartitionReassignmentsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getKError(m_error_code) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getNullableString(m_error_message) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t numTopics;
    if (pd.getArrayLength(numTopics) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (numTopics > 0)
    {
        for (int32_t i = 0; i < numTopics; ++i)
        {
            std::string topic;
            if (pd.getString(topic) != ErrNoError)
            {
                return ErrDecodeError;
            }

            int32_t ongoingPartitionReassignments;
            if (pd.getArrayLength(ongoingPartitionReassignments) != ErrNoError)
            {
                return ErrDecodeError;
            }

            auto &partitionMap = m_errors[topic];
            for (int32_t j = 0; j < ongoingPartitionReassignments; ++j)
            {
                int32_t partition;
                if (pd.getInt32(partition) != ErrNoError)
                {
                    return ErrDecodeError;
                }

                auto block = std::make_shared<AlterPartitionReassignmentsErrorBlock>();
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

int16_t AlterPartitionReassignmentsResponse::key() const
{
    return apiKeyAlterPartitionReassignments;
}

int16_t AlterPartitionReassignmentsResponse::version() const
{
    return m_version;
}

int16_t AlterPartitionReassignmentsResponse::header_version() const
{
    return 1;
}

bool AlterPartitionReassignmentsResponse::is_valid_version() const
{
    return m_version == 0;
}

bool AlterPartitionReassignmentsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool AlterPartitionReassignmentsResponse::is_flexible_version(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterPartitionReassignmentsResponse::required_version() const
{
    return V2_4_0_0;
}

std::chrono::milliseconds AlterPartitionReassignmentsResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}