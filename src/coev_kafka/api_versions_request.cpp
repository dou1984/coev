#include "version.h"
#include "api_versions_request.h"
#include "api_versions.h"



void ApiVersionsRequest::set_version(int16_t v)
{
    m_version = v;
}

int ApiVersionsRequest::encode(PEncoder &pe)
{
    if (m_version >= 3)
    {
        if (pe.putString(m_client_software_name) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pe.putString(m_client_software_version) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    return ErrNoError;
}

int ApiVersionsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    if (m_version >= 3)
    {
        if (pd.getString(m_client_software_name) != ErrNoError)
        {
            return ErrDecodeError;
        }
        if (pd.getString(m_client_software_version) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ApiVersionsRequest::key() const
{
    return apiKeyApiVersions;
}

int16_t ApiVersionsRequest::version() const
{
    return m_version;
}

int16_t ApiVersionsRequest::header_version() const
{
    return (m_version >= 3) ? 2 : 1;
}

bool ApiVersionsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 3;
}

bool ApiVersionsRequest::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool ApiVersionsRequest::isFlexibleVersion(int16_t version) const
{
    return version >= 3;
}

KafkaVersion ApiVersionsRequest::required_version() const
{
    switch (m_version)
    {
    case 3:
        return V2_4_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_10_0_0;
    default:
        return V2_4_0_0;
    }
}