#include "elect_leaders_request.h"
#include <vector>

void ElectLeadersRequest::set_version(int16_t v)
{
    m_version = v;
}

int ElectLeadersRequest::encode(packetEncoder &pe) const
{
    if (m_version > 0)
    {
        pe.putInt8(static_cast<int8_t>(m_type));
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_topic_partitions.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (const auto &kv : m_topic_partitions)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pe.putInt32Array(kv.second) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putDurationMs(m_timeout);
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int ElectLeadersRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    if (m_version > 0)
    {
        int8_t t;
        if (pd.getInt8(t) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_type = static_cast<ElectionType>(t);
    }

    int32_t topicCount;
    if (pd.getArrayLength(topicCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topic_partitions.clear();
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

            std::vector<int32_t> partitions(partitionCount);
            for (int32_t j = 0; j < partitionCount; ++j)
            {
                if (pd.getInt32(partitions[j]) != ErrNoError)
                {
                    return ErrDecodeError;
                }
            }

            m_topic_partitions[topic] = std::move(partitions);

            int32_t dummy;
            if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }

    if (pd.getDurationMs(m_timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t ElectLeadersRequest::key() const
{
    return apiKeyElectLeaders;
}

int16_t ElectLeadersRequest::version() const
{
    return m_version;
}

int16_t ElectLeadersRequest::header_version() const
{
    return 2;
}

bool ElectLeadersRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

bool ElectLeadersRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool ElectLeadersRequest::is_flexible_version(int16_t version)  const
{
    return version >= 2;
}

KafkaVersion ElectLeadersRequest::required_version()  const
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