#include "version.h"
#include "delete_topics_response.h"

void DeleteTopicsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DeleteTopicsResponse::encode(packet_encoder &pe) const
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_topic_error_codes.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &kv : m_topic_error_codes)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putInt16(static_cast<int16_t>(kv.second));
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DeleteTopicsResponse::decode(packet_decoder &pd, int16_t version)
{
    if (version >= 1)
    {

        if (pd.getDurationMs(m_throttle_time) != ErrNoError)
        {
            return ErrDecodeError;
        }

        m_version = version;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_topic_error_codes.clear();

    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int16_t errCode;
        if (pd.getInt16(errCode) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_topic_error_codes[topic] = static_cast<KError>(errCode);

        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DeleteTopicsResponse::key() const
{
    return apiKeyDeleteTopics;
}

int16_t DeleteTopicsResponse::version() const
{
    return m_version;
}

int16_t DeleteTopicsResponse::header_version() const
{
    if (m_version >= 4)
    {
        return 1;
    }
    return 0;
}

bool DeleteTopicsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DeleteTopicsResponse::is_flexible_version(int16_t version) const
{
    return version >= 4;
}

bool DeleteTopicsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

KafkaVersion DeleteTopicsResponse::required_version() const
{
    switch (m_version)
    {
    case 4:
        return V2_4_0_0;
    case 3:
        return V2_1_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_10_1_0;
    default:
        return V2_2_0_0;
    }
}

std::chrono::milliseconds DeleteTopicsResponse::throttle_time() const
{
    return m_throttle_time;
}