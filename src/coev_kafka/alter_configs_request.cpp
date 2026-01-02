#include "version.h"
#include "alter_configs_request.h"
#include "api_versions.h"

void AlterConfigsRequest::setVersion(int16_t v)
{
    Version = v;
}

int AlterConfigsRequest::encode(PEncoder &pe)
{
    if (!pe.putArrayLength(static_cast<int32_t>(Resources.size())))
    {
        return ErrEncodeError;
    }

    for (auto &r : Resources)
    {
        if (!r->encode(pe))
        {
            return ErrEncodeError;
        }
    }

    pe.putBool(ValidateOnly);
    return true;
}

int AlterConfigsRequest::decode(PDecoder &pd, int16_t version)
{
    int32_t resourceCount;
    if (!pd.getArrayLength(resourceCount))
    {
        return ErrDecodeError;
    }

    Resources.resize(resourceCount);
    for (int32_t i = 0; i < resourceCount; ++i)
    {
        Resources[i] = std::make_shared<AlterConfigsResource>();
        if (!Resources[i]->decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    bool validateOnly;
    if (!pd.getBool(validateOnly))
    {
        return ErrDecodeError;
    }
    ValidateOnly = validateOnly;

    return true;
}

int AlterConfigsResource::encode(PEncoder &pe)
{
    pe.putInt8(static_cast<int8_t>(Type));

    if (!pe.putString(Name))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(ConfigEntries.size())))
    {
        return ErrEncodeError;
    }

    for (auto &kv : ConfigEntries)
    {
        if (!pe.putString(kv.first))
        {
            return ErrEncodeError;
        }
        if (!pe.putNullableString(kv.second))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int AlterConfigsResource::decode(PDecoder &pd, int16_t version)
{
    int8_t t;
    if (!pd.getInt8(t))
    {
        return ErrDecodeError;
    }
    Type = static_cast<ConfigResourceType>(t);

    std::string name;
    if (!pd.getString(name))
    {
        return ErrDecodeError;
    }
    Name = name;

    int32_t n;
    if (!pd.getArrayLength(n))
    {
        return ErrDecodeError;
    }

    if (n > 0)
    {
        ConfigEntries.clear();
        for (int32_t i = 0; i < n; ++i)
        {
            std::string configKey;
            if (!pd.getString(configKey))
            {
                return ErrDecodeError;
            }
            std::string configValue;
            if (!pd.getNullableString(configValue))
            {
                return ErrDecodeError;
            }
            ConfigEntries[configKey] = configValue;
        }
    }

    return true;
}

int16_t AlterConfigsRequest::key() const
{
    return apiKeyAlterConfigs;
}

int16_t AlterConfigsRequest::version() const
{
    return Version;
}

int16_t AlterConfigsRequest::headerVersion() const
{
    return 1;
}

bool AlterConfigsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion AlterConfigsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    case 0:
        return V0_11_0_0;
    default:
        return V2_0_0_0;
    }
}