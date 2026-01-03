#include "version.h"
#include "describe_configs_response.h"

void DescribeConfigsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DescribeConfigsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (!pe.putArrayLength(static_cast<int32_t>(Resources.size())))
    {
        return ErrEncodeError;
    }

    for (auto &rr : Resources)
    {
        if (!rr->encode(pe, Version))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int DescribeConfigsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (pd.getDurationMs(ThrottleTime) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    Resources.clear();
    Resources.resize(n);

    for (int32_t i = 0; i < n; ++i)
    {
        Resources[i] = std::make_shared<ResourceResponse>();
        if (!Resources[i]->decode(pd, version))
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
    return Version;
}

int16_t DescribeConfigsResponse::headerVersion() const
{
    return 0;
}

bool DescribeConfigsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 2;
}

KafkaVersion DescribeConfigsResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds DescribeConfigsResponse::throttleTime() const
{
    return ThrottleTime;
}

int ResourceResponse::encode(PEncoder &pe, int16_t version) 
{
    pe.putInt16(ErrorCode);
    if (!pe.putString(ErrorMsg))
    {
        return ErrEncodeError;
    }
    pe.putInt8(static_cast<int8_t>(Type));
    if (!pe.putString(Name))
    {
        return ErrEncodeError;
    }

    if (!pe.putArrayLength(static_cast<int32_t>(Configs.size())))
    {
        return ErrEncodeError;
    }

    for (auto &c : Configs)
    {
        if (!c->encode(pe, version))
        {
            return ErrEncodeError;
        }
    }

    return true;
}

int ResourceResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getInt16(ErrorCode) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(ErrorMsg) != ErrNoError)
        return ErrDecodeError;

    int8_t t;
    if (pd.getInt8(t) != ErrNoError)
        return ErrDecodeError;
    Type = static_cast<ConfigResourceType>(t);

    if (pd.getString(Name) != ErrNoError)
        return ErrDecodeError;

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
        return ErrDecodeError;

    Configs.clear();
    Configs.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        Configs[i] = std::make_shared<ConfigEntry>();
        if (!Configs[i]->decode(pd, version))
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

// --- ConfigEntry ---

int ConfigEntry::encode(PEncoder &pe, int16_t version)
{
    if (!pe.putString(Name))
        return ErrEncodeError;
    if (!pe.putString(Value))
        return ErrEncodeError;
    pe.putBool(ReadOnly);

    if (version == 0)
    {
        pe.putBool(Default);
        pe.putBool(Sensitive);
    }
    else
    {
        pe.putInt8(static_cast<int8_t>(Source));
        pe.putBool(Sensitive);

        if (!pe.putArrayLength(static_cast<int32_t>(Synonyms.size())))
        {
            return ErrEncodeError;
        }
        for (auto &s : Synonyms)
        {
            if (!s->encode(pe, version))
            {
                return ErrEncodeError;
            }
        }
    }

    return true;
}

int ConfigEntry::decode(PDecoder &pd, int16_t version)
{
    if (version == 0)
    {
        Source = ConfigSource::SourceUnknown;
    }

    if (pd.getString(Name) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(Value) != ErrNoError)
        return ErrDecodeError;
    if (pd.getBool(ReadOnly) != ErrNoError)
        return ErrDecodeError;

    if (version == 0)
    {
        if (pd.getBool(Default) != ErrNoError)
            return ErrDecodeError;
        if (Default)
        {
            Source = ConfigSource::SourceDefault;
        }
    }
    else
    {
        int8_t src;
        if (pd.getInt8(src) != ErrNoError)
            return ErrDecodeError;
        Source = static_cast<ConfigSource>(src);
        Default = (Source == ConfigSource::SourceDefault);
    }

    if (pd.getBool(Sensitive) != ErrNoError)
        return ErrDecodeError;

    if (version > 0)
    {
        int32_t n;
        if (pd.getArrayLength(n) != ErrNoError)
            return ErrDecodeError;

        Synonyms.clear();
        Synonyms.resize(n);
        for (int32_t i = 0; i < n; ++i)
        {
            Synonyms[i] = std::make_shared<ConfigSynonym>();
            if (!Synonyms[i]->decode(pd, version))
            {
                return ErrDecodeError;
            }
        }
    }

    return ErrNoError;
}

int ConfigSynonym::encode(PEncoder &pe, int16_t /*version*/)
{
    if (!pe.putString(ConfigName))
        return ErrEncodeError;
    if (!pe.putString(ConfigValue))
        return ErrEncodeError;
    pe.putInt8(static_cast<int8_t>(Source));
    return true;
}

int ConfigSynonym::decode(PDecoder &pd, int16_t /*version*/)
{
    if (pd.getString(ConfigName) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(ConfigValue) != ErrNoError)
        return ErrDecodeError;

    int8_t src;
    if (pd.getInt8(src) != ErrNoError)
        return ErrDecodeError;
    Source = static_cast<ConfigSource>(src);
    return ErrNoError;
}