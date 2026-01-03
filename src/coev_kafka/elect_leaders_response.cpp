#include "version.h"
#include "elect_leaders_response.h"
#include <memory>

int PartitionResult::encode(PEncoder &pe, int16_t /*version*/)
{
    pe.putKError(ErrorCode);
    if (!pe.putNullableString(ErrorMessage))
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return true;
}

int PartitionResult::decode(PDecoder &pd, int16_t /*version*/)
{
    if (pd.getKError(ErrorCode) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pd.getNullableString(ErrorMessage) != ErrNoError)
    {
        return ErrEncodeError;
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrEncodeError;
    }
    return true;
}

// --- ElectLeadersResponse implementation ---
void ElectLeadersResponse::setVersion(int16_t v)
{
    Version = v;
}

int ElectLeadersResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (Version > 0)
    {
        pe.putKError(ErrorCode);
    }

    if (!pe.putArrayLength(static_cast<int32_t>(ReplicaElectionResults.size())))
    {
        return ErrEncodeError;
    }

    for (auto &topicEntry : ReplicaElectionResults)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;

        if (!pe.putString(topic))
        {
            return ErrEncodeError;
        }

        if (!pe.putArrayLength(static_cast<int32_t>(partitions.size())))
        {
            return ErrEncodeError;
        }

        for (auto &partEntry : partitions)
        {
            pe.putInt32(partEntry.first); // partition ID
            if (!partEntry.second->encode(pe, Version))
            {
                return ErrEncodeError;
            }
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int ElectLeadersResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (Version > 0)
    {
        if (pd.getKError(ErrorCode) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t numTopics;
    if (pd.getArrayLength(numTopics) != ErrNoError)
    {
        return ErrDecodeError;
    }

    ReplicaElectionResults.clear();
    ReplicaElectionResults.reserve(numTopics);

    for (int32_t i = 0; i < numTopics; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        int32_t numPartitions;
        if (pd.getArrayLength(numPartitions) != ErrNoError)
        {
            return ErrDecodeError;
        }

        auto &partitionMap = ReplicaElectionResults[topic];
        partitionMap.reserve(numPartitions);

        for (int32_t j = 0; j < numPartitions; ++j)
        {
            int32_t partition;
            if (pd.getInt32(partition) != ErrNoError)
            {
                return ErrDecodeError;
            }

            auto result = std::make_unique<PartitionResult>();
            if (!result->decode(pd, Version))
            {
                return ErrDecodeError;
            }

            partitionMap[partition] = std::move(result);
        }

        int32_t dummy;
        if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t ElectLeadersResponse::key() const
{
    return apiKeyElectLeaders;
}

int16_t ElectLeadersResponse::version() const
{
    return Version;
}

int16_t ElectLeadersResponse::headerVersion() const
{
    return 1; // Note: differs from request (which uses header v2)
}

bool ElectLeadersResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

bool ElectLeadersResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool ElectLeadersResponse::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

KafkaVersion ElectLeadersResponse::requiredVersion() const
{
    switch (Version)
    {
    case 2:
        return V2_4_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_10_0_0;
    default:
        return V2_4_0_0;
    }
}

std::chrono::milliseconds ElectLeadersResponse::throttleTime() const
{
    return ThrottleTime;
}