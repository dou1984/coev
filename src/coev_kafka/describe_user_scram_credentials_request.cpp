#include "describe_user_scram_credentials_request.h"

void DescribeUserScramCredentialsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DescribeUserScramCredentialsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(DescribeUsers.size())))
    {
        return ErrEncodeError;
    }

    for (auto &d : DescribeUsers)
    {
        if (!pe.putString(d.Name))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeUserScramCredentialsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (n == -1)
    {
        n = 0;
    }

    DescribeUsers.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        if (pd.getString(DescribeUsers[i].Name) != 0)
        {
            return ErrDecodeError;
        }

        int32_t dummy;
        if (pd.getEmptyTaggedFieldArray(dummy) != 0)
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != 0)
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
    return Version;
}

int16_t DescribeUserScramCredentialsRequest::headerVersion() const
{
    return 2;
}

bool DescribeUserScramCredentialsRequest::isValidVersion() const
{
    return Version == 0;
}

bool DescribeUserScramCredentialsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeUserScramCredentialsRequest::isFlexibleVersion(int16_t version)
{
    return version >= 0;
}

KafkaVersion DescribeUserScramCredentialsRequest::requiredVersion() const
{
    return V2_7_0_0;
}