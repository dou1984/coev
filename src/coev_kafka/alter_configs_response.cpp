#include "version.h"
#include "alter_configs_response.h"
#include "api_versions.h"

std::string AlterConfigError::Error() const
{
    std::string text = KErrorToString(m_err);
    if (!m_err_msg.empty())
    {
        text = text + " - " + m_err_msg;
    }
    return text;
}

int AlterConfigsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_resources.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &r : m_resources)
    {
        if (r.encode(pe) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    return ErrNoError;
}

int AlterConfigsResponse::decode(packet_decoder &pd, int16_t version)
{
    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t responseCount;
    if (pd.getArrayLength(responseCount) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_resources.resize(responseCount);
    for (int32_t i = 0; i < responseCount; ++i)
    {
        if (m_resources[i].decode(pd, version) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    return ErrNoError;
}

int AlterConfigsResourceResponse::encode(packet_encoder &pe) const
{
    pe.putInt16(m_code);
    if (pe.putString(m_message) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putInt8(static_cast<int8_t>(m_type));
    if (pe.putString(m_name) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int AlterConfigsResourceResponse::decode(packet_decoder &pd, int16_t version)
{
    int16_t errCode;
    if (pd.getInt16(errCode) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_code = errCode;

    if (pd.getNullableString(m_message) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int8_t t;
    if (pd.getInt8(t) != ErrNoError)
    {
        return ErrDecodeError;
    }
    m_type = static_cast<ConfigResourceType>(t);

    if (pd.getString(m_name) != ErrNoError)
    {
        return ErrDecodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

void AlterConfigsResponse::set_version(int16_t v)
{
    m_version = v;
}

int16_t AlterConfigsResponse::key() const
{
    return apiKeyAlterConfigs;
}

int16_t AlterConfigsResponse::version() const
{
    return m_version;
}

int16_t AlterConfigsResponse::header_version() const
{
    return 0;
}

bool AlterConfigsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion AlterConfigsResponse::required_version() const
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

std::chrono::milliseconds AlterConfigsResponse::throttle_time() const
{
    return m_throttle_time;
}