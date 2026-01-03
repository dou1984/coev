#include "version.h"
#include "alter_partition_reassignments_response.h"
#include "api_versions.h"

int alterPartitionReassignmentsErrorBlock::encode(PEncoder &pe)
{
    pe.putKError(errorCode);
    if (!pe.putNullableString(errorMessage))
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return true;
}

int alterPartitionReassignmentsErrorBlock::decode(PDecoder &pd)
{
    if (pd.getKError(errorCode) != ErrNoError)
    {
        return ErrEncodeError;
    }
    if (pd.getNullableString(errorMessage) != ErrNoError)
    {
        return ErrEncodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void AlterPartitionReassignmentsResponse::setVersion(int16_t v)
{
    Version = v;
}

void AlterPartitionReassignmentsResponse::AddError(const std::string &topic, int32_t partition, KError kerror, std::string message)
{
    Errors[topic][partition] = std::make_shared<alterPartitionReassignmentsErrorBlock>(kerror, message);
}

int AlterPartitionReassignmentsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putKError(ErrorCode);
    if (!pe.putNullableString(ErrorMessage))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Errors.size())))
    {
        return ErrEncodeError;
    }

    for (auto &topicPair : Errors)
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

int AlterPartitionReassignmentsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getKError(ErrorCode) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getNullableString(ErrorMessage) != ErrNoError)
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

            auto &partitionMap = Errors[topic];
            for (int32_t j = 0; j < ongoingPartitionReassignments; ++j)
            {
                int32_t partition;
                if (pd.getInt32(partition) != ErrNoError)
                {
                    return ErrDecodeError;
                }

                auto block = std::make_shared<alterPartitionReassignmentsErrorBlock>();
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

int16_t AlterPartitionReassignmentsResponse::key() const
{
    return apiKeyAlterPartitionReassignments;
}

int16_t AlterPartitionReassignmentsResponse::version() const
{
    return Version;
}

int16_t AlterPartitionReassignmentsResponse::headerVersion() const
{
    return 1;
}

bool AlterPartitionReassignmentsResponse::isValidVersion() const
{
    return Version == 0;
}

bool AlterPartitionReassignmentsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool AlterPartitionReassignmentsResponse::isFlexibleVersion(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterPartitionReassignmentsResponse::requiredVersion() const
{
    return V2_4_0_0;
}

std::chrono::milliseconds AlterPartitionReassignmentsResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}