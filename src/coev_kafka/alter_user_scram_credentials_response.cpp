#include "alter_user_scram_credentials_response.h"
#include "api_versions.h"

void AlterUserScramCredentialsResponse::set_version(int16_t v)
{
    m_version = v;
}

int AlterUserScramCredentialsResponse::encode(packetEncoder &pe)
{
     pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_results.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &u : m_results)
    {
        if (pe.putString(u->m_user) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putKError(u->m_error_code);
        if (pe.putNullableString(u->m_error_message) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterUserScramCredentialsResponse::decode(packetDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
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
        m_results.resize(numResults);
        for (int32_t i = 0; i < numResults; ++i)
        {
            auto result = std::make_shared<AlterUserScramCredentialsResult>();
            if (pd.getString(result->m_user) != ErrNoError)
            {
                return ErrEncodeError;
            }

            if (pd.getKError(result->m_error_code) != ErrNoError)
            {
                return ErrEncodeError;
            }

            if (pd.getNullableString(result->m_error_message) != ErrNoError)
            {
                return ErrEncodeError;
            }
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrEncodeError;
            }

            m_results[i] = result;
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
    return m_version;
}

int16_t AlterUserScramCredentialsResponse::header_version() const
{
    return 2;
}

bool AlterUserScramCredentialsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool AlterUserScramCredentialsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool AlterUserScramCredentialsResponse::is_flexible_version(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterUserScramCredentialsResponse::required_version() const
{
    return V2_7_0_0;
}

std::chrono::milliseconds AlterUserScramCredentialsResponse::throttle_time() const
{
    return m_throttle_time;
}