#include "version.h"
#include "incremental_alter_configs_request.h"
#include "api_versions.h"

int IncrementalAlterConfigsEntry::encode(PEncoder &pe)
{
    pe.putInt8(static_cast<int8_t>(Operation));

    int err = pe.putNullableString(Value);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int IncrementalAlterConfigsEntry::decode(PDecoder &pd, int16_t /*version*/)
{
    int8_t op;
    int err = pd.getInt8(op);
    if (err != 0)
        return err;
    Operation = static_cast<IncrementalAlterConfigsOperation>(op);

    err = pd.getNullableString(Value);
    if (err != 0)
        return err;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int IncrementalAlterConfigsResource::encode(PEncoder &pe)
{
    pe.putInt8(Type);

    int err = pe.putString(Name);
    if (err != 0)
        return err;

    err = pe.putArrayLength(static_cast<int32_t>(ConfigEntries.size()));
    if (err != 0)
        return err;

    for (auto &it : ConfigEntries)
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

int IncrementalAlterConfigsResource::decode(PDecoder &pd, int16_t version)
{
    int8_t t;
    int err = pd.getInt8(t);
    if (err != 0)
        return err;
    Type = static_cast<ConfigResourceType>(t);

    err = pd.getString(Name);
    if (err != 0)
        return err;

    int32_t n;
    err = pd.getArrayLength(n);
    if (err != 0)
        return err;

    ConfigEntries.clear();
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

            ConfigEntries.emplace(std::move(name), std::move(entry));
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void IncrementalAlterConfigsRequest::setVersion(int16_t v)
{
    Version = v;
}

int IncrementalAlterConfigsRequest::encode(PEncoder &pe)
{
    int err = pe.putArrayLength(static_cast<int32_t>(Resources.size()));
    if (err != 0)
        return err;

    for (auto &r : Resources)
    {
        err = r->encode(pe);
        if (err != 0)
            return err;
    }

    pe.putBool(ValidateOnly);

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int IncrementalAlterConfigsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int32_t count;
    int err = pd.getArrayLength(count);
    if (err != 0)
        return err;

    Resources.clear();
    Resources.reserve(count);
    for (int32_t i = 0; i < count; ++i)
    {
        auto r = std::make_unique<IncrementalAlterConfigsResource>();
        err = r->decode(pd, version);
        if (err != 0)
            return err;
        Resources.push_back(std::move(r));
    }

    bool validateOnly;
    err = pd.getBool(validateOnly);
    if (err != 0)
        return err;
    ValidateOnly = validateOnly;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t IncrementalAlterConfigsRequest::key() const
{
    return apiKeyIncrementalAlterConfigs;
}

int16_t IncrementalAlterConfigsRequest::version() const
{
    return Version;
}

int16_t IncrementalAlterConfigsRequest::headerVersion() const
{
    return (Version >= 1) ? 2 : 1;
}

bool IncrementalAlterConfigsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

bool IncrementalAlterConfigsRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool IncrementalAlterConfigsRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 1;
}

KafkaVersion IncrementalAlterConfigsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_4_0_0;
    default:
        return V2_3_0_0;
    }
}