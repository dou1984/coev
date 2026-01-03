#include "version.h"
#include "describe_client_quotas_response.h"

void DescribeClientQuotasResponse::setVersion(int16_t v)
{
    Version = v;
}

int DescribeClientQuotasResponse::encode(PEncoder &pe)
{

    pe.putDurationMs(ThrottleTime);

    pe.putInt16(static_cast<int16_t>(ErrorCode));

    if (!pe.putNullableString(ErrorMsg))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(Entries.size()))
    {
        return ErrEncodeError;
    }
    for (auto &e : Entries)
    {
        if (!e.encode(pe))
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int DescribeClientQuotasResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int32_t throttle;
    if (pd.getInt32(throttle) != ErrNoError)
    {
        return ErrDecodeError;
    }
    ThrottleTime = std::chrono::milliseconds(throttle);

    int16_t errorCode;
    if (pd.getInt16(errorCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    ErrorCode = static_cast<KError>(errorCode);

    if (pd.getNullableString(ErrorMsg) != ErrNoError)
    {

        return ErrDecodeError;
    }

    int32_t entryCount;
    if (pd.getArrayLength(entryCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Entries.clear();
    if (entryCount > 0)
    {
        Entries.resize(entryCount);
        for (int32_t i = 0; i < entryCount; ++i)
        {
            if (!Entries[i].decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }
    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int DescribeClientQuotasEntry::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Entity.size())))
    {
        return ErrEncodeError;
    }
    for (auto &comp : Entity)
    {
        if (!comp.encode(pe))
        {
            return ErrEncodeError;
        }
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Values.size())))
    {
        return ErrEncodeError;
    }
    for (auto &kv : Values)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        pe.putFloat64(kv.second);
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeClientQuotasEntry::decode(PDecoder &pd, int16_t version)
{
    int32_t componentCount;
    if (pd.getArrayLength(componentCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Entity.clear();
    if (componentCount > 0)
    {
        Entity.resize(componentCount);
        for (int32_t i = 0; i < componentCount; ++i)
        {
            if (!Entity[i].decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }

    int32_t valueCount;
    if (pd.getArrayLength(valueCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Values.clear();
    if (valueCount > 0)
    {
        for (int32_t i = 0; i < valueCount; ++i)
        {
            std::string key;
            if (pd.getString(key) != ErrNoError)
            {
                return ErrDecodeError;
            }
            double value;
            if (pd.getFloat64(value) != ErrNoError)
            {
                return ErrDecodeError;
            }
            Values[key] = value;
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }
    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int QuotaEntityComponent::encode(PEncoder &pe)
{
    if (!pe.putString(EntityType))
    {
        return ErrEncodeError;
    }

    if (MatchType == QuotaMatchType::QuotaMatchDefault)
    {
        if (!pe.putNullableString(nullptr))
        {
            return ErrEncodeError;
        }
    }
    else
    {
        if (!pe.putString(Name))
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
    ;
}

int QuotaEntityComponent::decode(PDecoder &pd, int16_t /*version*/)
{
    std::string entityType;
    if (pd.getString(entityType) != ErrNoError)
    {
        return ErrDecodeError;
    }
    EntityType = entityType;

    if (pd.getNullableString(Name) != ErrNoError)
    {
        return ErrDecodeError;
    }

    MatchType = Name.empty() ? QuotaMatchType::QuotaMatchDefault : QuotaMatchType::QuotaMatchExact;

    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeClientQuotasResponse::key() const
{
    return apiKeyDescribeClientQuotas;
}

int16_t DescribeClientQuotasResponse::version() const
{
    return Version;
}

int16_t DescribeClientQuotasResponse::headerVersion() const
{
    if (Version >= 1)
    {
        return 1;
    }
    return 0;
}

bool DescribeClientQuotasResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

bool DescribeClientQuotasResponse::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeClientQuotasResponse::isFlexibleVersion(int16_t version)
{
    return version >= 1;
}

KafkaVersion DescribeClientQuotasResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_8_0_0;
    case 0:
        return V2_6_0_0;
    default:
        return V2_8_0_0;
    }
}

std::chrono::milliseconds DescribeClientQuotasResponse::throttleTime() const
{
    return ThrottleTime;
}