#include "version.h"
#include "api_versions_request.h"
#include "api_versions.h"



void ApiVersionsRequest::setVersion(int16_t v)
{
    Version = v;
}

int ApiVersionsRequest::encode(PEncoder &pe)
{
    if (Version >= 3)
    {
        if (!pe.putString(ClientSoftwareName))
        {
            return ErrEncodeError;
        }
        if (!pe.putString(ClientSoftwareVersion))
        {
            return ErrEncodeError;
        }
        pe.putEmptyTaggedFieldArray();
    }

    return true;
}

int ApiVersionsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    if (Version >= 3)
    {
        if (pd.getString(ClientSoftwareName) != ErrNoError)
        {
            return ErrDecodeError;
        }
        if (pd.getString(ClientSoftwareVersion) != ErrNoError)
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
    return Version;
}

int16_t ApiVersionsRequest::headerVersion() const
{
    return (Version >= 3) ? 2 : 1;
}

bool ApiVersionsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 3;
}

bool ApiVersionsRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool ApiVersionsRequest::isFlexibleVersion(int16_t version) const
{
    return version >= 3;
}

KafkaVersion ApiVersionsRequest::requiredVersion() const
{
    switch (Version)
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