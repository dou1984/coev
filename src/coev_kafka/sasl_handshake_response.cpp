#include "version.h"
#include "sasl_handshake_response.h"
#include "api_versions.h"

void SaslHandshakeResponse::setVersion(int16_t v)
{
    Version = v;
}

int SaslHandshakeResponse::encode(PEncoder &pe)
{
    pe.putKError(Err);
    return pe.putStringArray(EnabledMechanisms);
}

int SaslHandshakeResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getKError(Err);
    if (err != 0)
    {
        return err;
    }

    return pd.getStringArray(EnabledMechanisms);
}

int16_t SaslHandshakeResponse::key()const{
    return apiKeySaslHandshake;
}

int16_t SaslHandshakeResponse::version()const
{
    return Version;
}

int16_t SaslHandshakeResponse::headerVersion() const
{
    return 0;
}

bool SaslHandshakeResponse::isValidVersion()const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion SaslHandshakeResponse::requiredVersion()const
{
    switch (Version)
    {
    case 1:
        return V1_0_0_0;
    default:
        return V0_10_0_0;
    }
}