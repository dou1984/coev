#include "version.h"
#include "heartbeat_response.h"
#include "api_versions.h"

void HeartbeatResponse::set_version(int16_t v)
{
    m_version = v;
}

int HeartbeatResponse::encode(packetEncoder &pe) const
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putKError(m_err);

    pe.putEmptyTaggedFieldArray();

    return 0;
}

int HeartbeatResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    if (m_version >= 1)
    {

        int err = pd.getDurationMs(m_throttle_time);
        if (err != 0)
            return err;
    }

    int err = pd.getKError(m_err);
    if (err != 0)
        return err;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t HeartbeatResponse::key() const
{
    return apiKeyHeartbeat;
}

int16_t HeartbeatResponse::version() const
{
    return m_version;
}

int16_t HeartbeatResponse::header_version() const
{
    // Response header version: 1 for flexible (v4+), otherwise 0
    return (m_version >= 4) ? 1 : 0;
}

bool HeartbeatResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool HeartbeatResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool HeartbeatResponse::is_flexible_version(int16_t ver)  const
{
    return ver >= 4;
}

KafkaVersion HeartbeatResponse::required_version()  const
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

std::chrono::milliseconds HeartbeatResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}