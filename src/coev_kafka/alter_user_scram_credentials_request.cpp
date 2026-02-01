#include "version.h"
#include "alter_user_scram_credentials_request.h"
#include "api_versions.h"
#include "scram_formatter.h"
void AlterUserScramCredentialsRequest::set_version(int16_t v)
{
    m_version = v;
}

int AlterUserScramCredentialsRequest::encode(packet_encoder &pe) const
{
    if (pe.putArrayLength(static_cast<int32_t>(m_deletions.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (const auto &d : m_deletions)
    {
        if (pe.putString(d.m_name) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putInt8(static_cast<int8_t>(d.m_mechanism));
        pe.putEmptyTaggedFieldArray();
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_upsertions.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (const auto &u : m_upsertions)
    {
        if (pe.putString(u.m_name) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putInt8(static_cast<int8_t>(u.m_mechanism));
        pe.putInt32(u.m_iterations);

        if (pe.putBytes(u.m_salt) != ErrNoError)
        {
            return ErrEncodeError;
        }

        // do not transmit the password over the wire
        ScramFormatter formatter(u.m_mechanism);
        std::string salted;
        if (!formatter.SaltedPassword(u.m_password, u.m_salt, static_cast<int>(u.m_iterations), salted))
        {
            return ErrEncodeError;
        }

        if (pe.putBytes(salted) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterUserScramCredentialsRequest::decode(packet_decoder &pd, int16_t version)
{
    int32_t numDeletions;
    if (pd.getArrayLength(numDeletions) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_deletions.resize(numDeletions);
    for (int32_t i = 0; i < numDeletions; ++i)
    {
        if (pd.getString(m_deletions[i].m_name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int8_t mechanism;
        if (pd.getInt8(mechanism) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_deletions[i].m_mechanism = static_cast<ScramMechanismType>(mechanism);
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t numUpsertions;
    if (pd.getArrayLength(numUpsertions) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_upsertions.resize(numUpsertions);
    for (int32_t i = 0; i < numUpsertions; ++i)
    {
        if (pd.getString(m_upsertions[i].m_name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int8_t mechanism;
        if (pd.getInt8(mechanism) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_upsertions[i].m_mechanism = static_cast<ScramMechanismType>(mechanism);

        if (pd.getInt32(m_upsertions[i].m_iterations) != ErrNoError)
        {
            return ErrDecodeError;
        }
        if (pd.getBytes(m_upsertions[i].m_salt) != ErrNoError)
        {
            return ErrDecodeError;
        }
        if (pd.getBytes(m_upsertions[i].m_salted_password) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t AlterUserScramCredentialsRequest::key() const
{
    return apiKeyAlterUserScramCredentials;
}

int16_t AlterUserScramCredentialsRequest::version() const
{
    return m_version;
}

int16_t AlterUserScramCredentialsRequest::header_version() const
{
    return 2;
}

bool AlterUserScramCredentialsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool AlterUserScramCredentialsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool AlterUserScramCredentialsRequest::is_flexible_version(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterUserScramCredentialsRequest::required_version() const
{
    return V2_7_0_0;
}