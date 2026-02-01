#include "version.h"
#include "api_versions.h"
#include "delete_topics_request.h"

void DeleteTopicsRequest::set_version(int16_t v)
{
    m_version = v;
}

static int16_t getVersionFromKafkaVersion(KafkaVersion kafkaVersion)
{
    if (kafkaVersion.IsAtLeast(V2_4_0_0))
        return 4;
    if (kafkaVersion.IsAtLeast(V2_1_0_0))
        return 3;
    if (kafkaVersion.IsAtLeast(V2_0_0_0))
        return 2;
    if (kafkaVersion.IsAtLeast(V0_11_0_0))
        return 1;
    return 0;
}

int DeleteTopicsRequest::encode(packet_encoder &pe) const
{
    if (pe.putStringArray(m_topics) != ErrNoError)
    {
        return ErrEncodeError;
    }

    pe.putDurationMs(m_timeout);

    if (is_flexible())
    {
        pe.putEmptyTaggedFieldArray();
    }

    return ErrNoError;
}

int DeleteTopicsRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getStringArray(m_topics) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t timeout;
    if (pd.getInt32(timeout) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_timeout = std::chrono::milliseconds(timeout);

    if (is_flexible_version(version))
    {
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

int16_t DeleteTopicsRequest::key() const
{
    return apiKeyDeleteTopics;
}

int16_t DeleteTopicsRequest::version() const
{
    return m_version;
}

int16_t DeleteTopicsRequest::header_version() const
{
    return (m_version >= 4) ? 2 : 1;
}

bool DeleteTopicsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DeleteTopicsRequest::is_flexible_version(int16_t version) const
{
    return version >= 4;
}

bool DeleteTopicsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

KafkaVersion DeleteTopicsRequest::required_version() const
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

DeleteTopicsRequest::DeleteTopicsRequest(KafkaVersion version, const std::vector<std::string> &topics, int64_t timeoutMs)
{
    m_version = getVersionFromKafkaVersion(version);
    m_topics = topics;
    m_timeout = std::chrono::milliseconds(timeoutMs);
}

