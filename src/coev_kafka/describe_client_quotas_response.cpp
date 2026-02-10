#include "version.h"
#include "describe_client_quotas_response.h"

void DescribeClientQuotasResponse::set_version(int16_t v)
{
    m_version = v;
}

int DescribeClientQuotasResponse::encode(packet_encoder &pe) const
{

    pe.putDurationMs(m_throttle_time);

    pe.putInt16(static_cast<int16_t>(m_error_code));

    if (pe.putNullableString(m_error_msg) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_entries.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &e : m_entries)
    {
        if (e.encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeClientQuotasResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    int32_t throttle;
    if (pd.getInt32(throttle) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_throttle_time = std::chrono::milliseconds(throttle);

    int16_t errorCode;
    if (pd.getInt16(errorCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_error_code = static_cast<KError>(errorCode);

    if (pd.getNullableString(m_error_msg) != ErrNoError)
    {

        return ErrDecodeError;
    }

    int32_t entryCount;
    if (pd.getArrayLength(entryCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_entries.clear();
    if (entryCount > 0)
    {
        m_entries.resize(entryCount);
        for (int32_t i = 0; i < entryCount; ++i)
        {
            if (!m_entries[i].decode(pd, version))
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

int DescribeClientQuotasEntry::encode(packet_encoder &pe) const
{
    if (pe.putArrayLength(static_cast<int32_t>(m_entity.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &comp : m_entity)
    {
        if (comp.encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_values.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &kv : m_values)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putFloat64(kv.second);
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeClientQuotasEntry::decode(packet_decoder &pd, int16_t version)
{
    int32_t component_count;
    if (pd.getArrayLength(component_count) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_entity.clear();
    if (component_count > 0)
    {
        m_entity.resize(component_count);
        for (int32_t i = 0; i < component_count; ++i)
        {
            if (!m_entity[i].decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }

    int32_t value_count;
    if (pd.getArrayLength(value_count) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_values.clear();
    if (value_count > 0)
    {
        for (int32_t i = 0; i < value_count; ++i)
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
            m_values[key] = value;
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

int QuotaEntityComponent::encode(packet_encoder &pe) const
{
    if (pe.putString(m_entity_type) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (m_match_type == QuotaMatchType::QuotaMatchDefault)
    {
        if (pe.putNullableString("") != ErrNoError)
        {
            return ErrEncodeError;
        }
    }
    else
    {
        if (pe.putString(m_name) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int QuotaEntityComponent::decode(packet_decoder &pd, int16_t /*version*/)
{
    std::string entity_type;
    if (pd.getString(entity_type) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_entity_type = entity_type;

    if (pd.getNullableString(m_name) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_match_type = m_name.empty() ? QuotaMatchType::QuotaMatchDefault : QuotaMatchType::QuotaMatchExact;

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
    return m_version;
}

int16_t DescribeClientQuotasResponse::header_version() const
{
    if (m_version >= 1)
    {
        return 1;
    }
    return 0;
}

bool DescribeClientQuotasResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool DescribeClientQuotasResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DescribeClientQuotasResponse::is_flexible_version(int16_t version) const
{
    return version >= 1;
}

KafkaVersion DescribeClientQuotasResponse::required_version() const
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

std::chrono::milliseconds DescribeClientQuotasResponse::throttle_time() const
{
    return m_throttle_time;
}