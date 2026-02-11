#include "version.h"
#include "sasl_authenticate_request.h"
#include "config.h"

SaslAuthenticateRequest::SaslAuthenticateRequest(std::shared_ptr<Config> &conf, const std::string &msg)
{
    m_sasl_auth_bytes = msg;
    if (conf->Version.IsAtLeast(V2_2_0_0))
    {
        m_version = 1;
    }
    else
    {
        m_version = 0;
    }
}

void SaslAuthenticateRequest::set_version(int16_t v)
{
    m_version = v;
}

int SaslAuthenticateRequest::encode(packet_encoder &pe) const
{
    return pe.putBytes(m_sasl_auth_bytes);
}

int SaslAuthenticateRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    return pd.getBytes(m_sasl_auth_bytes);
}

int16_t SaslAuthenticateRequest::key() const
{
    return apiKeySASLAuth;
}

int16_t SaslAuthenticateRequest::version() const
{
    return m_version;
}

int16_t SaslAuthenticateRequest::header_version() const
{
    return 1;
}

bool SaslAuthenticateRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion SaslAuthenticateRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_2_0_0;
    default:
        return V1_0_0_0;
    }
}