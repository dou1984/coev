#include "version.h"
#include "api_versions.h"
#include "sasl_authenticate_response.h"
#include <utility>

void SaslAuthenticateResponse::set_version(int16_t v)
{
    m_version = v;
}

int SaslAuthenticateResponse::encode(PEncoder &pe)
{
    pe.putKError(m_err);

    int err = pe.putNullableString(m_error_message);
    if (err != 0)
        return err;

    err = pe.putBytes(m_sasl_auth_bytes);
    if (err != 0)
        return err;

    if (m_version > 0)
    {
        pe.putInt64(m_session_lifetime_ms);
    }

    return 0;
}

int SaslAuthenticateResponse::decode(PDecoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getKError(m_err);
    if (err != 0)
        return err;

    std::string msg;
    err = pd.getNullableString(msg);
    if (err != 0)
        return err;

    err = pd.getBytes(m_sasl_auth_bytes);
    if (err != 0)
        return err;

    if (version > 0)
    {
        err = pd.getInt64(m_session_lifetime_ms);
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
    return m_version;
}

int16_t SaslAuthenticateResponse::header_version() const
{
    return 0;
}

bool SaslAuthenticateResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion SaslAuthenticateResponse::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_2_0_0;
    default:
        return V1_0_0_0;
    }
}