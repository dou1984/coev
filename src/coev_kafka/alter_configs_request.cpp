#include "version.h"
#include "alter_configs_request.h"
#include "api_versions.h"

void AlterConfigsRequest::set_version(int16_t v)
{
    m_version = v;
}

int AlterConfigsRequest::encode(packet_encoder &pe) const
{
    if (pe.putArrayLength(static_cast<int32_t>(m_resources.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &r : m_resources)
    {
        if (r->encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putBool(m_validate_only);
    return ErrNoError;
}

int AlterConfigsRequest::decode(packet_decoder &pd, int16_t version)
{
    int32_t resourceCount;
    if (pd.getArrayLength(resourceCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_resources.resize(resourceCount);
    for (int32_t i = 0; i < resourceCount; ++i)
    {
        m_resources[i] = std::make_shared<AlterConfigsResource>();
        if (m_resources[i]->decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    bool validateOnly;
    if (pd.getBool(validateOnly) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_validate_only = validateOnly;

    return ErrNoError;
}

int AlterConfigsResource::encode(packet_encoder &pe) const
{
    pe.putInt8(static_cast<int8_t>(m_type));

    if (pe.putString(m_name) != ErrNoError)
    {
        return ErrEncodeError;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_config_entries.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &kv : m_config_entries)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        if (pe.putNullableString(kv.second) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int AlterConfigsResource::decode(packet_decoder &pd, int16_t version)
{
    int8_t t;
    if (pd.getInt8(t) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_type = static_cast<ConfigResourceType>(t);

    std::string name;
    if (pd.getString(name) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_name = name;

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (n > 0)
    {
        m_config_entries.clear();
        for (int32_t i = 0; i < n; ++i)
        {
            std::string configKey;
            if (pd.getString(configKey) != ErrNoError)
            {
                return ErrDecodeError;
            }
            std::string configValue;
            if (pd.getNullableString(configValue) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_config_entries[configKey] = configValue;
        }
    }

    return ErrNoError;
}

int16_t AlterConfigsRequest::key() const
{
    return apiKeyAlterConfigs;
}

int16_t AlterConfigsRequest::version() const
{
    return m_version;
}

int16_t AlterConfigsRequest::header_version() const
{
    return 1;
}

bool AlterConfigsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion AlterConfigsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_0_0_0;
    }
}