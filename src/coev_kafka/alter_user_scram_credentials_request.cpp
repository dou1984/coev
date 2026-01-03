#include "version.h"
#include "alter_user_scram_credentials_request.h"
#include "api_versions.h"
#include "scram_formatter.h"
void AlterUserScramCredentialsRequest::setVersion(int16_t v)
{
    Version = v;
}

int AlterUserScramCredentialsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Deletions.size())))
    {
        return ErrEncodeError;
    }
    for (auto &d : Deletions)
    {
        if (!pe.putString(d.Name))
        {
            return ErrEncodeError;
        }
        pe.putInt8(static_cast<int8_t>(d.Mechanism));
        pe.putEmptyTaggedFieldArray();
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Upsertions.size())))
    {
        return ErrEncodeError;
    }
    for (auto &u : Upsertions)
    {
        if (!pe.putString(u.Name))
        {
            return ErrEncodeError;
        }
        pe.putInt8(static_cast<int8_t>(u.Mechanism));
        pe.putInt32(u.Iterations);

        if (!pe.putBytes(u.Salt))
        {
            return ErrEncodeError;
        }

        // do not transmit the password over the wire
        ScramFormatter formatter(u.Mechanism);
        std::string salted;
        if (!formatter.SaltedPassword(u.Password, u.Salt, static_cast<int>(u.Iterations), salted))
        {
            return ErrEncodeError;
        }

        if (!pe.putBytes(salted))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int AlterUserScramCredentialsRequest::decode(PDecoder &pd, int16_t version)
{
    int32_t numDeletions;
    if (pd.getArrayLength(numDeletions) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Deletions.resize(numDeletions);
    for (int32_t i = 0; i < numDeletions; ++i)
    {
        if (pd.getString(Deletions[i].Name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int8_t mechanism;
        if (pd.getInt8(mechanism) != ErrNoError)
        {
            return ErrDecodeError;
        }
        Deletions[i].Mechanism = static_cast<ScramMechanismType>(mechanism);
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

    Upsertions.resize(numUpsertions);
    for (int32_t i = 0; i < numUpsertions; ++i)
    {
        if (pd.getString(Upsertions[i].Name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        int8_t mechanism;
        if (pd.getInt8(mechanism) != ErrNoError)
        {
            return ErrDecodeError;
        }
        Upsertions[i].Mechanism = static_cast<ScramMechanismType>(mechanism);

        if (pd.getInt32(Upsertions[i].Iterations) != ErrNoError)
        {
            return ErrDecodeError;
        }
        if (pd.getBytes(Upsertions[i].Salt) != ErrNoError)
        {
            return ErrDecodeError;
        }
        if (pd.getBytes(Upsertions[i].saltedPassword) != ErrNoError)
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
    return Version;
}

int16_t AlterUserScramCredentialsRequest::headerVersion() const
{
    return 2;
}

bool AlterUserScramCredentialsRequest::isValidVersion() const
{
    return Version == 0;
}

bool AlterUserScramCredentialsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool AlterUserScramCredentialsRequest::isFlexibleVersion(int16_t version) const
{
    return version >= 0;
}

KafkaVersion AlterUserScramCredentialsRequest::requiredVersion() const
{
    return V2_7_0_0;
}