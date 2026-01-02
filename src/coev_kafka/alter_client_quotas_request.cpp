#include "version.h"
#include "api_versions.h"
#include "alter_client_quotas_request.h"

void AlterClientQuotasRequest::setVersion(int16_t v)
{
    Version = v;
}

int AlterClientQuotasRequest::encode(PEncoder &pe)
{
    // Entries
    if (!pe.putArrayLength(static_cast<int32_t>(Entries.size())))
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

    // ValidateOnly
    pe.putBool(ValidateOnly);

    return true;
}

int AlterClientQuotasRequest::decode(PDecoder &pd, int16_t version)
{
    // Entries
    int32_t entryCount;
    if (!pd.getArrayLength(entryCount))
    {
        return ErrDecodeError;
    }
    if (entryCount > 0)
    {
        Entries.resize(entryCount);
        for (auto &entry : Entries)
        {
            if (!entry.decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }
    else
    {
        Entries.clear();
    }

    // ValidateOnly
    bool validateOnly;
    if (!pd.getBool(validateOnly))
    {
        return ErrDecodeError;
    }
    ValidateOnly = validateOnly;

    return true;
}

int16_t AlterClientQuotasRequest::key() const
{
    return apiKeyAlterClientQuotas;
}

int16_t AlterClientQuotasRequest::version() const
{
    return Version;
}

int16_t AlterClientQuotasRequest::headerVersion() const
{
    return 1;
}

bool AlterClientQuotasRequest::isValidVersion() const
{
    return Version == 0;
}

KafkaVersion AlterClientQuotasRequest::requiredVersion() const
{
    return V2_6_0_0;
}

int AlterClientQuotasEntry::encode(PEncoder &pe)
{
    // Entity
    if (!pe.putArrayLength(static_cast<int32_t>(Entity.size())))
    {
        return ErrEncodeError;
    }
    for (auto &component : Entity)
    {
        if (!component.encode(pe))
        {
            return ErrEncodeError;
        }
    }

    // Ops
    if (!pe.putArrayLength(static_cast<int32_t>(Ops.size())))
    {
        return ErrEncodeError;
    }
    for (auto &o : Ops)
    {
        if (!o.encode(pe))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int AlterClientQuotasEntry::decode(PDecoder &pd, int16_t version)
{
    // Entity
    int32_t componentCount;
    if (!pd.getArrayLength(componentCount))
    {
        return ErrEncodeError;
    }
    if (componentCount > 0)
    {
        Entity.resize(componentCount);
        for (auto &component : Entity)
        {
            if (!component.decode(pd, version))
            {
                return ErrEncodeError;
            }
        }
    }
    else
    {
        Entity.clear();
    }

    // Ops
    int32_t opCount;
    if (!pd.getArrayLength(opCount))
    {
        return ErrEncodeError;
    }
    if (opCount > 0)
    {
        Ops.resize(opCount);
        for (auto &op : Ops)
        {
            if (!op.decode(pd, version))
            {
                return ErrEncodeError;
            }
        }
    }
    else
    {
        Ops.clear();
    }

    return true;
}

int ClientQuotasOp::encode(PEncoder &pe)
{
    // Key
    if (!pe.putString(Key))
    {
        return ErrEncodeError;
    }

    // Value
    pe.putFloat64(Value);

    // Remove
    pe.putBool(Remove);

    return true;
}

int ClientQuotasOp::decode(PDecoder &pd, int16_t version)
{
    // Key
    std::string key;
    if (!pd.getString(key))
    {
        return ErrEncodeError;
    }
    Key = key;

    // Value
    double value;
    if (!pd.getFloat64(value))
    {
        return ErrEncodeError;
    }
    Value = value;

    // Remove
    bool remove;
    if (!pd.getBool(remove))
    {
        return ErrEncodeError;
    }
    Remove = remove;

    return true;
}