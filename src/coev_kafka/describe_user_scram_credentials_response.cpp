#include "describe_user_scram_credentials_response.h"
#include <memory>

void DescribeUserScramCredentialsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DescribeUserScramCredentialsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putKError(ErrorCode);
    if (!pe.putNullableString(ErrorMessage))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Results.size())))
    {
        return ErrEncodeError;
    }

    for (auto &u : Results)
    {
        if (!pe.putString(u->User))
        {
            return ErrEncodeError;
        }
        pe.putKError(u->ErrorCode);
        if (!pe.putNullableString(u->ErrorMessage))
        {
            return ErrEncodeError;
        }

        if (!pe.putArrayLength(static_cast<int32_t>(u->CredentialInfos.size())))
        {
            return ErrEncodeError;
        }

        for (auto &c : u->CredentialInfos)
        {
            pe.putInt8(static_cast<int8_t>(c->Mechanism));
            pe.putInt32(c->Iterations);
            pe.putEmptyTaggedFieldArray();
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeUserScramCredentialsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getKError(ErrorCode) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getNullableString(ErrorMessage) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t numUsers;
    if (pd.getArrayLength(numUsers) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Results.clear();
    Results.reserve(numUsers);
    for (int32_t i = 0; i < numUsers; ++i)
    {
        auto result = std::make_unique<DescribeUserScramCredentialsResult>();

        if (pd.getString(result->User) != ErrNoError)
        {
            return ErrDecodeError;
        }

        if (pd.getKError(result->ErrorCode) != ErrNoError)
        {
            return ErrDecodeError;
        }

        if (pd.getNullableString(result->ErrorMessage) != ErrNoError)
        {
            return ErrDecodeError;
        }

        int32_t numCreds;
        if (pd.getArrayLength(numCreds) != ErrNoError)
        {
            return ErrDecodeError;
        }

        result->CredentialInfos.reserve(numCreds);
        for (int32_t j = 0; j < numCreds; ++j)
        {
            auto cred = std::make_shared<UserScramCredentialsResponseInfo>();

            int8_t mech;
            if (pd.getInt8(mech) != ErrNoError)
            {
                return ErrDecodeError;
            }
            cred->Mechanism = static_cast<ScramMechanismType>(mech);

            if (pd.getInt32(cred->Iterations) != ErrNoError)
            {
                return ErrDecodeError;
            }

            int32_t dummy;
            if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
            {
                return ErrDecodeError;
            }

            result->CredentialInfos.emplace_back(std::move(cred));
        }

        int32_t dummy;
        if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
        {
            return ErrDecodeError;
        }

        Results.emplace_back(std::move(result));
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }

    return ErrNoError;
}

int16_t DescribeUserScramCredentialsResponse::key() const
{
    return apiKeyDescribeUserScramCredentials;
}

int16_t DescribeUserScramCredentialsResponse::version() const
{
    return Version;
}

int16_t DescribeUserScramCredentialsResponse::headerVersion() const
{
    return 2;
}

bool DescribeUserScramCredentialsResponse::isValidVersion() const
{
    return Version == 0;
}

bool DescribeUserScramCredentialsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeUserScramCredentialsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 0; // all versions use flexible format (tagged fields)
}

KafkaVersion DescribeUserScramCredentialsResponse::requiredVersion() const
{
    return V2_7_0_0;
}

std::chrono::milliseconds DescribeUserScramCredentialsResponse::throttleTime() const
{
    return ThrottleTime;
}