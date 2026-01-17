#include "version.h"
#include "heartbeat_request.h"
#include "api_versions.h"

void HeartbeatRequest::set_version(int16_t v)
{
    m_version = v;
}

int HeartbeatRequest::encode(packetEncoder &pe)
{
    int err = pe.putString(m_group_id);
    if (err != 0)
        return err;

    pe.putInt32(m_generation_id);

    err = pe.putString(m_member_id);
    if (err != 0)
        return err;

    if (m_version >= 3)
    {
        err = pe.putNullableString(m_group_instance_id);
        if (err != 0)
        {
            return err;
        }
    }

    // Write empty tagged field array (required for flexible versions)
    pe.putEmptyTaggedFieldArray();

    return 0;
}

int HeartbeatRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getString(m_group_id);
    if (err != 0)
    {
        return err;
    }

    err = pd.getInt32(m_generation_id);
    if (err != 0)
    {
        return err;
    }

    err = pd.getString(m_member_id);
    if (err != 0)
    {
        return err;
    }

    if (m_version >= 3)
    {
        err = pd.getNullableString(m_group_instance_id);
        if (err != 0)
        {
            return err;
        }
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t HeartbeatRequest::key() const
{
    return apiKeyHeartbeat;
}

int16_t HeartbeatRequest::version() const
{
    return m_version;
}

int16_t HeartbeatRequest::header_version() const
{
    return (m_version >= 4) ? 2 : 1;
}

bool HeartbeatRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool HeartbeatRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool HeartbeatRequest::is_flexible_version(int16_t ver) const
{
    return ver >= 4;
}

KafkaVersion HeartbeatRequest::required_version() const
{
    switch (m_version)
    {
    case 4:
        return V2_4_0_0;
    case 3:
        return V2_3_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_8_2_0;
    default:
        return V2_3_0_0; // fallback
    }
}