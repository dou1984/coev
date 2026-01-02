#include "version.h"
#include "heartbeat_request.h"
#include "api_versions.h"

void HeartbeatRequest::setVersion(int16_t v)
{
    Version = v;
}

int HeartbeatRequest::encode(PEncoder &pe)
{
    int err = pe.putString(GroupId);
    if (err != 0)
        return err;

    pe.putInt32(GenerationId);

    err = pe.putString(MemberId);
    if (err != 0)
        return err;

    if (Version >= 3)
    {
        err = pe.putNullableString(GroupInstanceId);
        if (err != 0)
        {
            return err;
        }
    }

    // Write empty tagged field array (required for flexible versions)
    pe.putEmptyTaggedFieldArray();

    return 0;
}

int HeartbeatRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getString(GroupId);
    if (err != 0)
    {
        return err;
    }

    err = pd.getInt32(GenerationId);
    if (err != 0)
    {
        return err;
    }

    err = pd.getString(MemberId);
    if (err != 0)
    {
        return err;
    }

    if (Version >= 3)
    {
        err = pd.getNullableString(GroupInstanceId);
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
    return Version;
}

int16_t HeartbeatRequest::headerVersion() const
{
    return (Version >= 4) ? 2 : 1;
}

bool HeartbeatRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool HeartbeatRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool HeartbeatRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion HeartbeatRequest::requiredVersion() const
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