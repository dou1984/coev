#include "describe_user_scram_credentials_request.h"

void DescribeUserScramCredentialsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DescribeUserScramCredentialsRequest::encode(packetEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_describe_users.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &d : m_describe_users)
    {
        if (pe.putString(d.m_name) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeUserScramCredentialsRequest::decode(packetDecoder &pd, int16_t version)
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

    m_describe_users.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (pd.getString(m_describe_users[i].m_name) != ErrNoError)
        {
            return ErrDecodeError;
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

int16_t DescribeUserScramCredentialsRequest::key() const
{
    return apiKeyDescribeUserScramCredentials;
}

int16_t DescribeUserScramCredentialsRequest::version() const
{
    return m_version;
}

int16_t DescribeUserScramCredentialsRequest::header_version() const
{
    return 2;
}

bool DescribeUserScramCredentialsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool DescribeUserScramCredentialsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DescribeUserScramCredentialsRequest::is_flexible_version(int16_t version) const
{
    return version >= 0;
}

KafkaVersion DescribeUserScramCredentialsRequest::required_version() const
{
    return V2_7_0_0;
}