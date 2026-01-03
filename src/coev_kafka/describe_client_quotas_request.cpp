#include "describe_client_quotas_request.h"

void DescribeClientQuotasRequest::setVersion(int16_t v)
{
    Version = v;
}

int DescribeClientQuotasRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Components.size())))
    {
        return ErrEncodeError;
    }
    for (auto &c : Components)
    {
        if (!c.encode(pe))
        {
            return ErrEncodeError;
        }
    }

    pe.putBool(Strict);
    pe.putEmptyTaggedFieldArray();
    return true;
}

int DescribeClientQuotasRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int32_t componentCount;
    if (pd.getArrayLength(componentCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Components.clear();
    if (componentCount > 0)
    {
        Components.resize(componentCount);
        for (int32_t i = 0; i < componentCount; ++i)
        {
            if (!Components[i].decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }

    if (pd.getBool(Strict) != ErrNoError)
    {
        return ErrDecodeError;
    }
    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int QuotaFilterComponent::encode(PEncoder &pe)
{
    if (!pe.putString(EntityType))
    {
        return ErrEncodeError;
    }

    pe.putInt8(static_cast<int8_t>(MatchType));

    if (MatchType == QuotaMatchType::QuotaMatchAny || MatchType == QuotaMatchType::QuotaMatchDefault)
    {
        if (!pe.putNullableString(nullptr))
        {
            return ErrEncodeError;
        }
    }
    else
    {
        if (!pe.putString(Match))
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return true;
}

int QuotaFilterComponent::decode(PDecoder &pd, int16_t /*version*/)
{
    std::string entityType;
    if (pd.getString(entityType) != ErrNoError)
    {
        return ErrDecodeError;
    }
    EntityType = entityType;

    int8_t matchType;
    if (pd.getInt8(matchType) != ErrNoError)
    {
        return ErrDecodeError;
    }
    MatchType = static_cast<QuotaMatchType>(matchType);

    if (pd.getNullableString(Match) != ErrNoError)
    {

        return ErrDecodeError;
    }
    int32_t _;
    if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeClientQuotasRequest::key() const
{
    return apiKeyDescribeClientQuotas;
}

int16_t DescribeClientQuotasRequest::version() const
{
    return Version;
}

int16_t DescribeClientQuotasRequest::headerVersion() const
{
    if (Version >= 1)
    {
        return 2;
    }
    return 1;
}

bool DescribeClientQuotasRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

bool DescribeClientQuotasRequest::isFlexible() const
{
    return isFlexibleVersion(Version);
}

bool DescribeClientQuotasRequest::isFlexibleVersion(int16_t version)
{
    return version >= 1;
}

KafkaVersion DescribeClientQuotasRequest::requiredVersion() const
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

std::shared_ptr<DescribeClientQuotasRequest> NewDescribeClientQuotasRequest(KafkaVersion kafkaVersion, const std::vector<QuotaFilterComponent> &components, bool strict)
{
    auto d = std::make_shared<DescribeClientQuotasRequest>();
    d->Components = components;
    d->Strict = strict;
    if (kafkaVersion.IsAtLeast(V2_8_0_0))
    {
        d->Version = 1;
    }
    else
    {
        d->Version = 0;
    }
    return d;
}