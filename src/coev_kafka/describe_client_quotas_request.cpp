#include "describe_client_quotas_request.h"

void DescribeClientQuotasRequest::set_version(int16_t v)
{
    m_version = v;
}

int DescribeClientQuotasRequest::encode(packetEncoder &pe) const
{
    if (pe.putArrayLength(static_cast<int32_t>(m_components.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &c : m_components)
    {
        if (c.encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putBool(m_strict);
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeClientQuotasRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int32_t componentCount;
    if (pd.getArrayLength(componentCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_components.clear();
    if (componentCount > 0)
    {
        m_components.resize(componentCount);
        for (int32_t i = 0; i < componentCount; ++i)
        {
            if (!m_components[i].decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }

    if (pd.getBool(m_strict) != ErrNoError)
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

int QuotaFilterComponent::encode(packetEncoder &pe) const
{
    if (pe.putString(m_entity_type) != ErrNoError)
    {
        return ErrEncodeError;
    }

    pe.putInt8(static_cast<int8_t>(m_match_type));

    if (m_match_type == QuotaMatchType::QuotaMatchAny || m_match_type == QuotaMatchType::QuotaMatchDefault)
    {
        if (pe.putNullableString(nullptr) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }
    else
    {
        if (pe.putString(m_match) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int QuotaFilterComponent::decode(packetDecoder &pd, int16_t /*version*/)
{
    std::string entityType;
    if (pd.getString(entityType) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_entity_type = entityType;

    int8_t matchType;
    if (pd.getInt8(matchType) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_match_type = static_cast<QuotaMatchType>(matchType);

    if (pd.getNullableString(m_match) != ErrNoError)
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
    return m_version;
}

int16_t DescribeClientQuotasRequest::header_version() const
{
    if (m_version >= 1)
    {
        return 2;
    }
    return 1;
}

bool DescribeClientQuotasRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool DescribeClientQuotasRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DescribeClientQuotasRequest::is_flexible_version(int16_t version) const
{
    return version >= 1;
}

KafkaVersion DescribeClientQuotasRequest::required_version() const
{
    switch (m_version)
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
    d->m_components = components;
    d->m_strict = strict;
    if (kafkaVersion.IsAtLeast(V2_8_0_0))
    {
        d->m_version = 1;
    }
    else
    {
        d->m_version = 0;
    }
    return d;
}