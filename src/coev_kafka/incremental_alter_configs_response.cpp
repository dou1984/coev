#include "version.h"
#include "incremental_alter_configs_response.h"
#include "alter_configs_response.h"

void IncrementalAlterConfigsResponse::set_version(int16_t v)
{
    m_version = v;
}

int IncrementalAlterConfigsResponse::encode(packet_encoder &pe) const
{

    pe.putDurationMs(m_throttle_time);

    int err = pe.putArrayLength(static_cast<int32_t>(m_resources.size()));
    if (err != 0)
        return err;

    for (auto &res : m_resources)
    {
        err = res.encode(pe);
        if (err != 0)
            return err;
    }

    if (is_flexible_version(m_version))
    {
        pe.putEmptyTaggedFieldArray();
    }

    return 0;
}

int IncrementalAlterConfigsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getDurationMs(m_throttle_time);
    if (err != 0)
        return err;

    int32_t count;
    err = pd.getArrayLength(count);
    if (err != 0)
        return err;

    m_resources.clear();
    m_resources.resize(count);
    for (int32_t i = 0; i < count; ++i)
    {
        err = m_resources[i].decode(pd, version);
        if (err != 0)
            return err;
    }

    if (is_flexible_version(version))
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
    return m_version;
}

int16_t IncrementalAlterConfigsResponse::header_version() const
{
    return (m_version >= 1) ? 1 : 0;
}

bool IncrementalAlterConfigsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

bool IncrementalAlterConfigsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool IncrementalAlterConfigsResponse::is_flexible_version(int16_t ver) const
{
    return ver >= 1;
}

KafkaVersion IncrementalAlterConfigsResponse::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_4_0_0;
    default:
        return V2_3_0_0;
    }
}

std::chrono::milliseconds IncrementalAlterConfigsResponse::throttle_time() const
{
    return m_throttle_time;
}