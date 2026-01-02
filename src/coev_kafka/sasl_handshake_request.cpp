#include "version.h"
#include "sasl_handshake_request.h"
#include "api_versions.h"

void SaslHandshakeRequest::setVersion(int16_t v)
{
    Version = v;
}

int SaslHandshakeRequest::encode(PEncoder &pe)
{
    return pe.putString(Mechanism);
}

int SaslHandshakeRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    return pd.getString(Mechanism);
}

int16_t SaslHandshakeRequest::key() const
{
    return apiKeySaslHandshake;
}

int16_t SaslHandshakeRequest::version() const
{
    return Version;
}

int16_t SaslHandshakeRequest::headerVersion() const
{
    return 1;
}

bool SaslHandshakeRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion SaslHandshakeRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V1_0_0_0;
    default:
        return V0_10_0_0;
    }
}