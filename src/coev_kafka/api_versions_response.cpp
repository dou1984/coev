#include "version.h"
#include "api_versions_response.h"
#include "api_versions.h"

int ApiVersionsResponseKey::encode(PEncoder &pe, int16_t version)
{
    pe.putInt16(ApiKey);
    pe.putInt16(MinVersion);
    pe.putInt16(MaxVersion);

    if (version >= 3)
    {
        pe.putEmptyTaggedFieldArray();
    }

    return true;
}

int ApiVersionsResponseKey::decode(PDecoder &pd, int16_t version)
{
    if (pd.getInt16(ApiKey) != ErrNoError)
        return ErrEncodeError;
    if (pd.getInt16(MinVersion) != ErrNoError)
        return ErrEncodeError;
    if (pd.getInt16(MaxVersion) != ErrNoError)
        return ErrEncodeError;

    if (version >= 3)
    {
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            return ErrEncodeError;
    }

    return true;
}

// --------------------------
// ApiVersionsResponse
// --------------------------

void ApiVersionsResponse::setVersion(int16_t v)
{
    Version = v;
}

int ApiVersionsResponse::encode(PEncoder &pe)
{
    pe.putInt16(ErrorCode);

    if (!pe.putArrayLength(static_cast<int32_t>(ApiKeys.size())))
    {
        return ErrEncodeError;
    }

    for (auto &block : ApiKeys)
    {
        if (!block.encode(pe, Version))
        {
            return ErrEncodeError;
        }
    }

    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    if (Version >= 3)
    {
        pe.putEmptyTaggedFieldArray();
    }

    return true;
}

PDecoder &ApiVersionsResponse::downgradeFlexibleDecoder(PDecoder &pd)
{
    // auto _ = dynamic_cast<realFlexibleDecoder>(pd);
    // pd.setFlexible(false);
    return pd;
}

int ApiVersionsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (pd.getInt16(ErrorCode) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (ErrorCode == ErrUnsupportedVersion)
    {
        Version = 0;
        pd = downgradeFlexibleDecoder(pd);
    }

    int32_t numApiKeys;
    if (pd.getArrayLength(numApiKeys) != ErrNoError)
    {
        return ErrDecodeError;
    }

    ApiKeys.resize(numApiKeys);
    for (int32_t i = 0; i < numApiKeys; ++i)
    {
        if (!ApiKeys[i].decode(pd, Version))
        {
            return ErrDecodeError;
        }
    }

    if (Version >= 1)
    {
        if (pd.getDurationMs(ThrottleTime) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    if (Version >= 3)
    {
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return true;
}

int16_t ApiVersionsResponse::key() const
{
    return apiKeyApiVersions;
}

int16_t ApiVersionsResponse::version() const
{
    return Version;
}

int16_t ApiVersionsResponse::headerVersion() const
{
    // Per KIP-511, ApiVersionsResponse always uses v0 response header
    return 0;
}

bool ApiVersionsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 3;
}

bool ApiVersionsResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool ApiVersionsResponse::isFlexibleVersion(int16_t version) const
{
    return version >= 3;
}

KafkaVersion ApiVersionsResponse::requiredVersion() const
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

std::chrono::milliseconds ApiVersionsResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}