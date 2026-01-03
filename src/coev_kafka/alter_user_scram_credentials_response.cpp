#include "alter_user_scram_credentials_response.h"
#include "api_versions.h"

void AlterUserScramCredentialsResponse::setVersion(int16_t v)
{
    Version = v;
}

int AlterUserScramCredentialsResponse::encode(PEncoder &pe)
{
     pe.putDurationMs(ThrottleTime);

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
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int AlterUserScramCredentialsResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrEncodeError;
    }

    int32_t numResults;
    if (pd.getArrayLength(numResults) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (numResults > 0)
    {
        Results.resize(numResults);
        for (int32_t i = 0; i < numResults; ++i)
        {
            auto result = std::make_shared<AlterUserScramCredentialsResult>();
            if (pd.getString(result->User) != ErrNoError)
            {
                return ErrEncodeError;
            }

            if (pd.getKError(result->ErrorCode) != ErrNoError)
            {
                return ErrEncodeError;
            }

            if (pd.getNullableString(result->ErrorMessage) != ErrNoError)
            {
                return ErrEncodeError;
            }
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrEncodeError;
            }

            Results[i] = result;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t AlterUserScramCredentialsResponse::key() const
{
    return apiKeyAlterUserScramCredentials;
}

int16_t AlterUserScramCredentialsResponse::version() const
{
    return Version;
}

int16_t AlterUserScramCredentialsResponse::headerVersion() const
{
    return 2;
}

bool AlterUserScramCredentialsResponse::isValidVersion() const
{
    return Version == 0;
}

bool AlterUserScramCredentialsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool AlterUserScramCredentialsResponse::isFlexibleVersion(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterUserScramCredentialsResponse::requiredVersion() const
{
    return V2_7_0_0;
}

std::chrono::milliseconds AlterUserScramCredentialsResponse::throttleTime() const
{
    return ThrottleTime;
}