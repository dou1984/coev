#include "version.h"
#include "incremental_alter_configs_response.h"
#include "alter_configs_response.h"

void IncrementalAlterConfigsResponse::setVersion(int16_t v)
{
    Version = v;
}

int IncrementalAlterConfigsResponse::encode(PEncoder &pe)
{

    pe.putDurationMs(ThrottleTime);

    int err = pe.putArrayLength(static_cast<int32_t>(Resources.size()));
    if (err != 0)
        return err;

    for (auto &res : Resources)
    {
        err = res->encode(pe);
        if (err != 0)
            return err;
    }

    // Note: In flexible version (v1+), tagged fields are appended AFTER the array.
    // But according to Kafka protocol spec for this response:
    // - v0: no tagged fields
    // - v1: has empty tagged field array at end
    // So we add it only if flexible
    if (isFlexibleVersion(Version))
    {
        pe.putEmptyTaggedFieldArray();
    }

    return 0;
}

int IncrementalAlterConfigsResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getDurationMs(ThrottleTime);
    if (err != 0)
        return err;

    int32_t count;
    err = pd.getArrayLength(count);
    if (err != 0)
        return err;

    Resources.clear();
    Resources.reserve(count);
    for (int32_t i = 0; i < count; ++i)
    {
        auto res = std::make_shared<AlterConfigsResourceResponse>();
        err = res->decode(pd, version);
        if (err != 0)
            return err;
        Resources.push_back(std::move(res));
    }

    if (isFlexibleVersion(version))
    {
        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
            return err;
    }

    return 0;
}

int16_t IncrementalAlterConfigsResponse::key() const
{
    return apiKeyIncrementalAlterConfigs;
}

int16_t IncrementalAlterConfigsResponse::version() const
{
    return Version;
}

int16_t IncrementalAlterConfigsResponse::headerVersion() const
{
    return (Version >= 1) ? 1 : 0;
}

bool IncrementalAlterConfigsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

bool IncrementalAlterConfigsResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool IncrementalAlterConfigsResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 1;
}

KafkaVersion IncrementalAlterConfigsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_4_0_0;
    default:
        return V2_3_0_0;
    }
}

std::chrono::milliseconds IncrementalAlterConfigsResponse::throttleTime() const
{
    return ThrottleTime;
}