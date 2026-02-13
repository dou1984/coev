#include "version.h"
#include "api_versions_response.h"
#include "api_versions.h"

int ApiVersionsResponseKey::encode(packet_encoder &pe, int16_t version) const
{
    pe.putInt16(m_api_key);
    pe.putInt16(m_min_version);
    pe.putInt16(m_max_version);

    if (version >= 3)
    {
        pe.putEmptyTaggedFieldArray();
    }

    return ErrNoError;
}

int ApiVersionsResponseKey::decode(packet_decoder &pd, int16_t version)
{
    if (pd.getInt16(m_api_key) != ErrNoError)
        return ErrDecodeError;
    if (pd.getInt16(m_min_version) != ErrNoError)
        return ErrDecodeError;
    if (pd.getInt16(m_max_version) != ErrNoError)
        return ErrDecodeError;

    if (version >= 3)
    {
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            return ErrDecodeError;
    }

    return ErrNoError;
}

void ApiVersionsResponse::set_version(int16_t v)
{
    m_version = v;
}

int ApiVersionsResponse::encode(packet_encoder &pe) const
{
    pe.putInt16(m_code);

    if (pe.putArrayLength(static_cast<int32_t>(m_api_keys.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &block : m_api_keys)
    {
        if (block.encode(pe, m_version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    if (m_version >= 3)
    {
        pe.putEmptyTaggedFieldArray();
    }

    return ErrNoError;
}

packet_decoder &ApiVersionsResponse::downgrade_flexible_decoder(packet_decoder &pd)
{
    if (pd._is_flexible())
    {
        pd._pop_flexible();
    }
    return pd;
}

int ApiVersionsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getInt16(m_code) != ErrNoError)
    {
        return ErrDecodeError;
    }

    if (m_code == ErrUnsupportedVersion)
    {
        m_version = 0;
        downgrade_flexible_decoder(pd);
    }

    int32_t numApiKeys;
    if (pd.getArrayLength(numApiKeys) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_api_keys.resize(numApiKeys);
    for (int32_t i = 0; i < numApiKeys; ++i)
    {
        if (m_api_keys[i].decode(pd, m_version) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    if (m_version >= 1)
    {
        if (pd.getDurationMs(m_throttle_time) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    if (m_version >= 3)
    {
        int32_t _;
        if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

int16_t ApiVersionsResponse::key() const
{
    return apiKeyApiVersions;
}

int16_t ApiVersionsResponse::version() const
{
    return m_version;
}

int16_t ApiVersionsResponse::header_version() const
{
    return 0;
}

bool ApiVersionsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 3;
}

bool ApiVersionsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool ApiVersionsResponse::is_flexible_version(int16_t version) const
{
    return version >= 3;
}

KafkaVersion ApiVersionsResponse::required_version() const
{
    switch (m_version)
    {
    case 3:
        return V2_4_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_10_0_0;
    default:
        return V2_4_0_0;
    }
}

std::chrono::milliseconds ApiVersionsResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}