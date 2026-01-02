#include "version.h"
#include "sasl_authenticate_request.h"

void SaslAuthenticateRequest::setVersion(int16_t v)
{
    Version = v;
}

int SaslAuthenticateRequest::encode(PEncoder &pe)
{
    return pe.putBytes(SaslAuthBytes);
}

int SaslAuthenticateRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    return pd.getBytes(SaslAuthBytes);
}

int16_t SaslAuthenticateRequest::key() const
{
    return apiKeySASLAuth;
}

int16_t SaslAuthenticateRequest::version() const
{
    return Version;
}

int16_t SaslAuthenticateRequest::headerVersion() const
{
    return 1;
}

bool SaslAuthenticateRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion SaslAuthenticateRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_2_0_0;
    default:
        return V1_0_0_0;
    }
}