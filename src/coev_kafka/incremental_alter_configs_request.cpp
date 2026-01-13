#include "version.h"
#include "incremental_alter_configs_request.h"
#include "api_versions.h"

int IncrementalAlterConfigsEntry::encode(packetEncoder &pe)
{
    pe.putInt8(static_cast<int8_t>(m_operation));

    int err = pe.putNullableString(m_value);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int IncrementalAlterConfigsEntry::decode(packetDecoder &pd, int16_t /*version*/)
{
    int8_t op;
    int err = pd.getInt8(op);
    if (err != 0)
        return err;
    m_operation = static_cast<IncrementalAlterConfigsOperation>(op);

    err = pd.getNullableString(m_value);
    if (err != 0)
        return err;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int IncrementalAlterConfigsResource::encode(packetEncoder &pe)
{
    pe.putInt8(m_type);

    int err = pe.putString(m_name);
    if (err != 0)
        return err;

    err = pe.putArrayLength(static_cast<int32_t>(m_config_entries.size()));
    if (err != 0)
        return err;

    for (auto &it : m_config_entries)
    {
        err = pe.putString(it.first);
        if (err != 0)
            return err;

        err = it.second.encode(pe);
        if (err != 0)
            return err;
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int IncrementalAlterConfigsResource::decode(packetDecoder &pd, int16_t version)
{
    int8_t t;
    int err = pd.getInt8(t);
    if (err != 0)
        return err;
    m_type = static_cast<ConfigResourceType>(t);

    err = pd.getString(m_name);
    if (err != 0)
        return err;

    int32_t n;
    err = pd.getArrayLength(n);
    if (err != 0)
        return err;

    m_config_entries.clear();
    if (n > 0)
    {
        // ConfigEntries.reserve(n);
        for (int32_t i = 0; i < n; ++i)
        {
            std::string name;
            err = pd.getString(name);
            if (err != 0)
                return err;

            IncrementalAlterConfigsEntry entry;
            err = entry.decode(pd, version);
            if (err != 0)
                return err;

            m_config_entries.emplace(std::move(name), std::move(entry));
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void IncrementalAlterConfigsRequest::set_version(int16_t v)
{
    m_version = v;
}

int IncrementalAlterConfigsRequest::encode(packetEncoder &pe)
{
    int err = pe.putArrayLength(static_cast<int32_t>(m_resources.size()));
    if (err != 0)
        return err;

    for (auto &r : m_resources)
    {
        err = r->encode(pe);
        if (err != 0)
            return err;
    }

    pe.putBool(m_validate_only);

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int IncrementalAlterConfigsRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int32_t count;
    int err = pd.getArrayLength(count);
    if (err != 0)
        return err;

    m_resources.clear();
    m_resources.reserve(count);
    for (int32_t i = 0; i < count; ++i)
    {
        auto r = std::make_unique<IncrementalAlterConfigsResource>();
        err = r->decode(pd, version);
        if (err != 0)
            return err;
        m_resources.push_back(std::move(r));
    }

    bool validateOnly;
    err = pd.getBool(validateOnly);
    if (err != 0)
        return err;
    m_validate_only = validateOnly;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t IncrementalAlterConfigsRequest::key() const
{
    return apiKeyIncrementalAlterConfigs;
}

int16_t IncrementalAlterConfigsRequest::version() const
{
    return m_version;
}

int16_t IncrementalAlterConfigsRequest::header_version() const
{
    return (m_version >= 1) ? 2 : 1;
}

bool IncrementalAlterConfigsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool IncrementalAlterConfigsRequest::is_flexible()
{
    return is_flexible_version(m_version);
}

bool IncrementalAlterConfigsRequest::is_flexible_version(int16_t ver)
{
    return ver >= 1;
}

KafkaVersion IncrementalAlterConfigsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_4_0_0;
    default:
        return V2_3_0_0;
    }
}