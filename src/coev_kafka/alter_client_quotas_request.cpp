#include "version.h"
#include "api_versions.h"
#include "alter_client_quotas_request.h"

void AlterClientQuotasRequest::set_version(int16_t v)
{
    m_version = v;
}

int AlterClientQuotasRequest::encode(packetEncoder &pe)
{
    // Entries
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

    // ValidateOnly
    pe.putBool(m_validate_only);

    return ErrNoError;
}

int AlterClientQuotasRequest::decode(packetDecoder &pd, int16_t version)
{
    // Entries
    int32_t entryCount;
    if (pd.getArrayLength(entryCount) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (entryCount > 0)
    {
        m_entries.resize(entryCount);
        for (auto &entry : m_entries)
        {
            if (entry.decode(pd, version) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }
    else
    {
        m_entries.clear();
    }

    // ValidateOnly
    bool validateOnly;
    if (pd.getBool(validateOnly) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_validate_only = validateOnly;

    return ErrNoError;
}

int16_t AlterClientQuotasRequest::key() const
{
    return apiKeyAlterClientQuotas;
}

int16_t AlterClientQuotasRequest::version() const
{
    return m_version;
}

int16_t AlterClientQuotasRequest::header_version() const
{
    return 1;
}

bool AlterClientQuotasRequest::is_valid_version() const
{
    return m_version == 0;
}

KafkaVersion AlterClientQuotasRequest::required_version() const
{
    return V2_6_0_0;
}

int AlterClientQuotasEntry::encode(packetEncoder &pe)
{
    // Entity
    if (pe.putArrayLength(static_cast<int32_t>(m_entity.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &component : m_entity)
    {
        if (component.encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    // Ops
    if (pe.putArrayLength(static_cast<int32_t>(m_ops.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }
    for (auto &o : m_ops)
    {
        if (o.encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int AlterClientQuotasEntry::decode(packetDecoder &pd, int16_t version)
{
    // Entity
    int32_t componentCount;
    if (pd.getArrayLength(componentCount) != ErrNoError)
    {
        return ErrEncodeError;
    }
    if (componentCount > 0)
    {
        m_entity.resize(componentCount);
        for (auto &component : m_entity)
        {
            if (component.decode(pd, version) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }
    }
    else
    {
        m_entity.clear();
    }

    // Ops
    int32_t opCount;
    if (pd.getArrayLength(opCount) != ErrNoError)
    {
        return ErrEncodeError;
    }
    if (opCount > 0)
    {
        m_ops.resize(opCount);
        for (auto &op : m_ops)
        {
            if (op.decode(pd, version) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }
    }
    else
    {
        m_ops.clear();
    }

    return ErrNoError;
}

int ClientQuotasOp::encode(packetEncoder &pe)
{
    // Key
    if (pe.putString(m_key) != ErrNoError)
    {
        return ErrEncodeError;
    }

    // Value
    pe.putFloat64(m_value);

    // Remove
    pe.putBool(m_remove);

    return ErrNoError;
}

int ClientQuotasOp::decode(packetDecoder &pd, int16_t version)
{
    // Key
    std::string key;
    if (pd.getString(key) != ErrNoError)
    {
        return ErrEncodeError;
    }
    m_key = key;

    // Value
    double value;
    if (pd.getFloat64(value) != ErrNoError)
    {
        return ErrEncodeError;
    }
    m_value = value;

    // Remove
    bool remove;
    if (pd.getBool(remove) != ErrNoError)
    {
        return ErrEncodeError;
    }
    m_remove = remove;

    return ErrNoError;
}