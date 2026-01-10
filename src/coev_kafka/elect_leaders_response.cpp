#include "version.h"
#include "elect_leaders_response.h"
#include <memory>

int PartitionResult::encode(PEncoder &pe, int16_t /*version*/)
{
    pe.putKError(m_error_code);
    if (pe.putNullableString(m_error_message) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int PartitionResult::decode(PDecoder &pd, int16_t /*version*/)
{
    if (pd.getKError(m_error_code) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pd.getNullableString(m_error_message) != ErrNoError)
    {
        return ErrEncodeError;
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrEncodeError;
    }
    return ErrNoError;
}

// --- ElectLeadersResponse implementation ---
void ElectLeadersResponse::set_version(int16_t v)
{
    m_version = v;
}

int ElectLeadersResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);

    if (m_version > 0)
    {
        pe.putKError(m_error_code);
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_replica_election_results.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &topicEntry : m_replica_election_results)
    {
        auto &topic = topicEntry.first;
        auto &partitions = topicEntry.second;

        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (pe.putArrayLength(static_cast<int32_t>(partitions.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }

        for (auto &partEntry : partitions)
        {
            pe.putInt32(partEntry.first); // partition ID
            if (partEntry.second->encode(pe, m_version) != ErrNoError)
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
    m_version = version;
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (m_version > 0)
    {
        if (pd.getKError(m_error_code) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t numTopics;
    if (pd.getArrayLength(numTopics) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_replica_election_results.clear();
    m_replica_election_results.reserve(numTopics);

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

        auto &partitionMap = m_replica_election_results[topic];
        partitionMap.reserve(numPartitions);

        for (int32_t j = 0; j < numPartitions; ++j)
        {
            int32_t partition;
            if (pd.getInt32(partition) != ErrNoError)
            {
                return ErrDecodeError;
            }

            auto result = std::make_unique<PartitionResult>();
            if (result->decode(pd, m_version) != ErrNoError)
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
    return m_version;
}

int16_t ElectLeadersResponse::headerVersion() const
{
    return 1; // Note: differs from request (which uses header v2)
}

bool ElectLeadersResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

bool ElectLeadersResponse::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool ElectLeadersResponse::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

KafkaVersion ElectLeadersResponse::required_version() const
{
    switch (m_version)
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
    return m_throttle_time;
}