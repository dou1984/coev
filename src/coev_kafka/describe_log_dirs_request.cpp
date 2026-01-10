#include "version.h"
#include "describe_log_dirs_request.h"

void DescribeLogDirsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DescribeLogDirsRequest::encode(PEncoder &pe)
{
    int32_t length = static_cast<int32_t>(m_describe_topics.size());
    if (length == 0)
    {
        length = -1;
    }
    if (pe.putArrayLength(length) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &d : m_describe_topics)
    {
        if (pe.putString(d.m_topic) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pe.putInt32Array(d.m_partition_ids) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeLogDirsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (n == -1)
    {
        n = 0;
    }

    std::vector<DescribeLogDirsRequestTopic> topics(n);
    for (int32_t i = 0; i < n; ++i)
    {
        std::string topic;
        if (pd.getString(topic) != ErrNoError)
        {
            return ErrDecodeError;
        }
        topics[i].m_topic = topic;

        if (pd.getInt32Array(topics[i].m_partition_ids) != ErrNoError)
        {
            return ErrDecodeError;
        }

        int32_t dummy;
        if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    m_describe_topics = std::move(topics);

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeLogDirsRequest::key() const
{
    return apiKeyDescribeLogDirs;
}

int16_t DescribeLogDirsRequest::version() const
{
    return m_version;
}

int16_t DescribeLogDirsRequest::headerVersion() const
{
    if (m_version >= 2)
    {
        return 2;
    }
    return 1;
}

bool DescribeLogDirsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool DescribeLogDirsRequest::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool DescribeLogDirsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 2;
}

KafkaVersion DescribeLogDirsRequest::required_version() const
{
    switch (m_version)
    {
    case 4:
        return V3_3_0_0;
    case 3:
        return V3_2_0_0;
    case 2:
        return V2_6_0_0;
    case 1:
        return V2_0_0_0;
    default:
        return V1_0_0_0;
    }
}