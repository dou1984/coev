#include "version.h"
#include "sasl_handshake_request.h"
#include "api_versions.h"

void SaslHandshakeRequest::set_version(int16_t v)
{
    m_version = v;
}

int SaslHandshakeRequest::encode(packetEncoder &pe)
{
    return pe.putString(m_mechanism);
}

int SaslHandshakeRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    return pd.getString(m_mechanism);
}

int16_t SaslHandshakeRequest::key() const
{
    return apiKeySaslHandshake;
}

int16_t SaslHandshakeRequest::version() const
{
    return m_version;
}

int16_t SaslHandshakeRequest::header_version() const
{
    return 1;
}

bool SaslHandshakeRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion SaslHandshakeRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V1_0_0_0;
    default:
        return V0_10_0_0;
    }
}