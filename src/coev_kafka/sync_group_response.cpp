#include "version.h"
#include "sync_group_response.h"
#include "api_versions.h"

void SyncGroupResponse::setVersion(int16_t v)
{
    Version = v;
}

int SyncGroupResponse::GetMemberAssignment(std::shared_ptr<ConsumerGroupMemberAssignment> &assignment)
{
    assignment = std::make_shared<ConsumerGroupMemberAssignment>();
    return ::decode(MemberAssignment, std::dynamic_pointer_cast<IDecoder>(assignment), nullptr);
}

int SyncGroupResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }
    pe.putKError(Err);
    int err = pe.putBytes(MemberAssignment);
    if (err != 0)
    {
        return err;
    }
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int SyncGroupResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int err = 0;
    if (Version >= 1)
    {

        err = pd.getDurationMs(ThrottleTime);
        if (err != 0)
        {
            return err;
        }
    }
    err = pd.getKError(Err);
    if (err != 0)
    {
        return err;
    }
    err = pd.getBytes(MemberAssignment);
    if (err != 0)
    {
        return err;
    }
    int32_t _;
    err = pd.getEmptyTaggedFieldArray(_);
    return err;
}

int16_t SyncGroupResponse::key() const
{
    return apiKeySyncGroup;
}

int16_t SyncGroupResponse::version() const
{
    return Version;
}

int16_t SyncGroupResponse::headerVersion() const
{
    if (Version >= 4)
    {
        return 1;
    }
    return 0;
}

bool SyncGroupResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool SyncGroupResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool SyncGroupResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion SyncGroupResponse::requiredVersion() const
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
        return V0_9_0_0;
    default:
        return V2_3_0_0;
    }
}

std::chrono::milliseconds SyncGroupResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}