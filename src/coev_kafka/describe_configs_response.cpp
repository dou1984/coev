#include "version.h"
#include "describe_configs_response.h"

void DescribeConfigsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DescribeConfigsResponse::encode(packetEncoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_resources.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &rr : m_resources)
    {
        if (rr->encode(pe, m_version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int DescribeConfigsResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_resources.clear();
    m_resources.resize(n);

    for (int32_t i = 0; i < n; ++i)
    {
        m_resources[i] = std::make_shared<ResourceResponse>();
        if (m_resources[i]->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

int16_t DescribeConfigsResponse::key() const
{
    return apiKeyDescribeConfigs;
}

int16_t DescribeConfigsResponse::version() const
{
    return m_version;
}

int16_t DescribeConfigsResponse::header_version() const
{
    return 0;
}

bool DescribeConfigsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

KafkaVersion DescribeConfigsResponse::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_0_0_0;
    case 1:
        return V1_1_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_0_0_0;
    }
}

std::chrono::milliseconds DescribeConfigsResponse::throttle_time() const
{
    return m_throttle_time;
}

int ResourceResponse::encode(packetEncoder &pe, int16_t version) const
{
    pe.putInt16(m_error_code);
    if (pe.putString(m_error_msg) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putInt8(static_cast<int8_t>(m_type));
    if (pe.putString(m_name) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_configs.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &c : m_configs)
    {
        if (c->encode(pe, version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int ResourceResponse::decode(packetDecoder &pd, int16_t version)
{
    if (pd.getInt16(m_error_code) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_error_msg) != ErrNoError)
        return ErrDecodeError;

    int8_t t;
    if (pd.getInt8(t) != ErrNoError)
        return ErrDecodeError;
    m_type = static_cast<ConfigResourceType>(t);

    if (pd.getString(m_name) != ErrNoError)
        return ErrDecodeError;

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
        return ErrDecodeError;

    m_configs.clear();
    m_configs.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_configs[i] = std::make_shared<ConfigEntry>();
        if (m_configs[i]->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

// --- ConfigEntry ---

int ConfigEntry::encode(packetEncoder &pe, int16_t version) const
{
    if (pe.putString(m_name) != ErrNoError)
        return ErrEncodeError;
    if (pe.putString(m_value) != ErrNoError)
        return ErrEncodeError;
    pe.putBool(m_read_only);

    if (version == 0)
    {
        pe.putBool(m_default);
        pe.putBool(m_sensitive);
    }
    else
    {
        pe.putInt8(static_cast<int8_t>(m_source));
        pe.putBool(m_sensitive);

        if (pe.putArrayLength(static_cast<int32_t>(m_synonyms.size())) != ErrNoError)
        {
            return ErrEncodeError;
        }
        for (auto &s : m_synonyms)
        {
            if (s->encode(pe, version) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }
    }

    return ErrNoError;
}

int ConfigEntry::decode(packetDecoder &pd, int16_t version)
{
    if (version == 0)
    {
        m_source = ConfigSource::SourceUnknown;
    }

    if (pd.getString(m_name) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_value) != ErrNoError)
        return ErrDecodeError;
    if (pd.getBool(m_read_only) != ErrNoError)
        return ErrDecodeError;

    if (version == 0)
    {
        if (pd.getBool(m_default) != ErrNoError)
            return ErrDecodeError;
        if (m_default)
        {
            m_source = ConfigSource::SourceDefault;
        }
    }
    else
    {
        int8_t src;
        if (pd.getInt8(src) != ErrNoError)
            return ErrDecodeError;
        m_source = static_cast<ConfigSource>(src);
        m_default = (m_source == ConfigSource::SourceDefault);
    }

    if (pd.getBool(m_sensitive) != ErrNoError)
        return ErrDecodeError;

    if (version > 0)
    {
        int32_t n;
        if (pd.getArrayLength(n) != ErrNoError)
            return ErrDecodeError;

        m_synonyms.clear();
        m_synonyms.resize(n);
        for (int32_t i = 0; i < n; ++i)
        {
            m_synonyms[i] = std::make_shared<ConfigSynonym>();
            if (m_synonyms[i]->decode(pd, version) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }

    return ErrNoError;
}

int ConfigSynonym::encode(packetEncoder &pe, int16_t /*version*/) const
{
    if (pe.putString(m_config_name) != ErrNoError)
        return ErrEncodeError;
    if (pe.putString(m_config_value) != ErrNoError)
        return ErrEncodeError;
    pe.putInt8(static_cast<int8_t>(m_source));
    return ErrNoError;
}

int ConfigSynonym::decode(packetDecoder &pd, int16_t /*version*/)
{
    if (pd.getString(m_config_name) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_config_value) != ErrNoError)
        return ErrDecodeError;

    int8_t src;
    if (pd.getInt8(src) != ErrNoError)
        return ErrDecodeError;
    m_source = static_cast<ConfigSource>(src);
    return ErrNoError;
}