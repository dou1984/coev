#include "create_partitions_response.h"
#include <sstream>

void CreatePartitionsResponse::set_version(int16_t v)
{
    m_version = v;
}

int CreatePartitionsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_topic_partition_errors.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &pair : m_topic_partition_errors)
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

    return ErrNoError;
}

int CreatePartitionsResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topic_partition_errors.clear();
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        auto err = std::make_shared<TopicPartitionError>();
        if (err->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_topic_partition_errors[topic] = err;
    }

    return ErrNoError;
}

int16_t CreatePartitionsResponse::key()const{
    return 37; // apiKeyCreatePartitions
}

int16_t CreatePartitionsResponse::version()const
{
    return m_version;
}

int16_t CreatePartitionsResponse::header_version()const
{
    return 0;
}

bool CreatePartitionsResponse::is_valid_version()const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion CreatePartitionsResponse::required_version() const
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

std::chrono::milliseconds CreatePartitionsResponse::throttle_time() const
{
    return m_throttle_time;
}

std::string TopicPartitionError::Error() const
{
    std::string text = ""; // In real code, this would map KError to string
    if (!m_err_msg.empty())
    {
        text += " - " + m_err_msg;
    }
    return text;
}

int TopicPartitionError::encode(PEncoder &pe)
{
    pe.putKError(m_err);
    if (pe.putNullableString(m_err_msg) != ErrNoError)
    {
        return ErrEncodeError;
    }
    return ErrNoError;
}

int TopicPartitionError::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(m_err) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pd.getNullableString(m_err_msg) != ErrNoError)
    {
        return ErrEncodeError;
    }

    return ErrNoError;
}