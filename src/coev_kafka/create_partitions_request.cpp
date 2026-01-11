#include "create_partitions_request.h"
#include "api_versions.h"
void CreatePartitionsRequest::set_version(int16_t v)
{
    m_version = v;
}

int CreatePartitionsRequest::encode(PEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_topic_partitions.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &pair : m_topic_partitions)
    {
        if (pe.putString(pair.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pair.second->encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putInt32(static_cast<int32_t>(m_timeout.count()));
    pe.putBool(m_validate_only);

    return ErrNoError;
}

int CreatePartitionsRequest::decode(PDecoder &pd, int16_t version)
{
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topic_partitions.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto partition = std::make_shared<TopicPartition>();
        if (partition->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_topic_partitions[topic] = partition;
    }

    if (pd.getDurationMs(m_timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getBool(m_validate_only) != ErrNoError)
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t CreatePartitionsRequest::key() const
{
    return apiKeyCreatePartitions;
}

int16_t CreatePartitionsRequest::version() const
{
    return m_version;
}

int16_t CreatePartitionsRequest::header_version() const
{
    return 1;
}

bool CreatePartitionsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion CreatePartitionsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    case 0:
        return V1_0_0_0;
    default:
        return V2_0_0_0;
    }
}

std::chrono::milliseconds CreatePartitionsRequest::throttle_time() const
{
    return m_timeout;
}