#include "version.h"
#include "elect_leaders_response.h"
#include <memory>

int PartitionResult::encode(packet_encoder &pe, int16_t /*version*/) const
{
    pe.putKError(m_code);
    if (pe.putNullableString(m_message) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int PartitionResult::decode(packet_decoder &pd, int16_t /*version*/)
{
    if (pd.getKError(m_code) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pd.getNullableString(m_message) != ErrNoError)
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

int ElectLeadersResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (m_version > 0)
    {
        pe.putKError(m_code);
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_replica_election_results.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &[topic, partitions] : m_replica_election_results)
    {

        if (pe.putString(topic) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (pe.putArrayLength(static_cast<int32_t>(partitions.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }

        for (auto &[pid, partition] : partitions)
        {
            pe.putInt32(pid); // partition ID
            if (partition.encode(pe, m_version) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int ElectLeadersResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (m_version > 0)
    {
        if (pd.getKError(m_code) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t num_topics;
    if (pd.getArrayLength(num_topics) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_replica_election_results.clear();
    m_replica_election_results.reserve(num_topics);

    for (int32_t i = 0; i < num_topics; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }

        int32_t num_partitions;
        if (pd.getArrayLength(num_partitions) != ErrNoError)
        {
            return ErrDecodeError;
        }

        auto &partitions = m_replica_election_results[topic];
        for (int32_t j = 0; j < num_partitions; ++j)
        {
            int32_t partition;
            if (pd.getInt32(partition) != ErrNoError)
            {
                return ErrDecodeError;
            }

            if (partitions[partition].decode(pd, m_version) != ErrNoError)
            {
                return ErrDecodeError;
            }
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

int16_t ElectLeadersResponse::header_version() const
{
    return 1; // Note: differs from request (which uses header v2)
}

bool ElectLeadersResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

bool ElectLeadersResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool ElectLeadersResponse::is_flexible_version(int16_t version) const
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

std::chrono::milliseconds ElectLeadersResponse::throttle_time() const
{
    return m_throttle_time;
}