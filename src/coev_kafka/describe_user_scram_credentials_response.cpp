#include "describe_user_scram_credentials_response.h"
#include <memory>

void DescribeUserScramCredentialsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DescribeUserScramCredentialsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);
    pe.putKError(m_error_code);
    if (pe.putNullableString(m_error_message) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_results.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &u : m_results)
    {
        if (pe.putString(u.m_user) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putKError(u.m_error_code);
        if (pe.putNullableString(u.m_error_message) != ErrNoError)
        {
            return ErrEncodeError;
        }

        if (pe.putArrayLength(static_cast<int32_t>(u.m_credential_infos.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }

        for (auto &c : u.m_credential_infos)
        {
            pe.putInt8(static_cast<int8_t>(c.m_mechanism));
            pe.putInt32(c.m_iterations);
            pe.putEmptyTaggedFieldArray();
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeUserScramCredentialsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getKError(m_error_code) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (pd.getNullableString(m_error_message) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t num_users;
    if (pd.getArrayLength(num_users) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_results.clear();
    m_results.reserve(num_users);
    for (int32_t i = 0; i < num_users; ++i)
    {
        auto &result = m_results[i];
        if (pd.getString(result.m_user) != ErrNoError)
        {
            return ErrDecodeError;
        }

        if (pd.getKError(result.m_error_code) != ErrNoError)
        {
            return ErrDecodeError;
        }

        if (pd.getNullableString(result.m_error_message) != ErrNoError)
        {
            return ErrDecodeError;
        }

        int32_t num_creds;
        if (pd.getArrayLength(num_creds) != ErrNoError)
        {
            return ErrDecodeError;
        }

        result.m_credential_infos.reserve(num_creds);
        for (int32_t j = 0; j < num_creds; ++j)
        {

            int8_t mech;
            if (pd.getInt8(mech) != ErrNoError)
            {
                return ErrDecodeError;
            }
            auto &info = result.m_credential_infos[j];
            info.m_mechanism = static_cast<ScramMechanismType>(mech);

            if (pd.getInt32(info.m_iterations) != ErrNoError)
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
    return m_version;
}

int16_t DescribeUserScramCredentialsResponse::header_version() const
{
    return 2;
}

bool DescribeUserScramCredentialsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool DescribeUserScramCredentialsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DescribeUserScramCredentialsResponse::is_flexible_version(int16_t version) const
{
    return version >= 0; // all versions use flexible format (tagged fields)
}

KafkaVersion DescribeUserScramCredentialsResponse::required_version() const
{
    return V2_7_0_0;
}

std::chrono::milliseconds DescribeUserScramCredentialsResponse::throttle_time() const
{
    return m_throttle_time;
}