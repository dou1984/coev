#include "version.h"
#include "api_versions.h"
#include "sasl_authenticate_response.h"
#include <utility>

void SaslAuthenticateResponse::setVersion(int16_t v)
{
    Version = v;
}

int SaslAuthenticateResponse::encode(PEncoder &pe)
{
    pe.putKError(Err);

    int err = pe.putNullableString(ErrorMessage);
    if (err != 0)
        return err;

    err = pe.putBytes(SaslAuthBytes);
    if (err != 0)
        return err;

    if (Version > 0)
    {
        pe.putInt64(SessionLifetimeMs);
    }

    return 0;
}

int SaslAuthenticateResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getKError(Err);
    if (err != 0)
        return err;

    std::string msg;
    err = pd.getNullableString(msg);
    if (err != 0)
        return err;

    err = pd.getBytes(SaslAuthBytes);
    if (err != 0)
        return err;

    if (version > 0)
    {
        err = pd.getInt64(SessionLifetimeMs);
        if (err != 0)
            return err;
    }

    return 0;
}

int16_t SaslAuthenticateResponse::key() const
{
    return apiKeySASLAuth;
}

int16_t SaslAuthenticateResponse::version() const
{
    return Version;
}

int16_t SaslAuthenticateResponse::headerVersion() const
{
    return 0;
}

bool SaslAuthenticateResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion SaslAuthenticateResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_2_0_0;
    default:
        return V1_0_0_0;
    }
}