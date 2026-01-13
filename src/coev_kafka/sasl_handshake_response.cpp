#include "version.h"
#include "sasl_handshake_response.h"
#include "api_versions.h"

void SaslHandshakeResponse::set_version(int16_t v)
{
    m_version = v;
}

int SaslHandshakeResponse::encode(packetEncoder &pe)
{
    pe.putKError(m_err);
    return pe.putStringArray(m_enabled_mechanisms);
}

int SaslHandshakeResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getKError(m_err);
    if (err != 0)
    {
        return err;
    }

    return pd.getStringArray(m_enabled_mechanisms);
}

int16_t SaslHandshakeResponse::key()const{
    return apiKeySaslHandshake;
}

int16_t SaslHandshakeResponse::version()const
{
    return m_version;
}

int16_t SaslHandshakeResponse::header_version() const
{
    return 0;
}

bool SaslHandshakeResponse::is_valid_version()const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion SaslHandshakeResponse::required_version()const
{
    switch (m_version)
    {
    case 1:
        return V1_0_0_0;
    default:
        return V0_10_0_0;
    }
}