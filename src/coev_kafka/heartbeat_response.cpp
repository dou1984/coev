#include "version.h"
#include "heartbeat_response.h"
#include "api_versions.h"

void HeartbeatResponse::setVersion(int16_t v)
{
    Version = v;
}

int HeartbeatResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putKError(Err);

    pe.putEmptyTaggedFieldArray();

    return 0;
}

int HeartbeatResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (Version >= 1)
    {

        int err = pd.getDurationMs(ThrottleTime);
        if (err != 0)
            return err;
    }

    int err = pd.getKError(Err);
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
    return Version;
}

int16_t HeartbeatResponse::headerVersion() const
{
    // Response header version: 1 for flexible (v4+), otherwise 0
    return (Version >= 4) ? 1 : 0;
}

bool HeartbeatResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool HeartbeatResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool HeartbeatResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion HeartbeatResponse::requiredVersion() const
{
    switch (Version)
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

std::chrono::milliseconds HeartbeatResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}