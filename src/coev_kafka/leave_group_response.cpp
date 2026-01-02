#include "version.h"
#include "leave_group_response.h"

void LeaveGroupResponse::setVersion(int16_t v)
{
    Version = v;
}

int LeaveGroupResponse::encode(PEncoder &pe)
{
    if (Version >= 1)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putKError(Err);

    if (Version >= 3)
    {
        int err = pe.putArrayLength(static_cast<int32_t>(Members.size()));
        if (err != 0)
            return err;

        for (auto &member : Members)
        {
            err = pe.putString(member.MemberId);
            if (err != 0)
                return err;

            err = pe.putNullableString(member.GroupInstanceId);
            if (err != 0)
                return err;

            pe.putKError(member.Err);
            pe.putEmptyTaggedFieldArray();
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int LeaveGroupResponse::decode(PDecoder &pd, int16_t version)
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

    if (Version >= 3)
    {
        int32_t membersLen;
        err = pd.getArrayLength(membersLen);
        if (err != 0)
            return err;

        Members.resize(membersLen);
        for (int32_t i = 0; i < membersLen; ++i)
        {
            err = pd.getString(Members[i].MemberId);
            if (err != 0)
                return err;

            err = pd.getNullableString(Members[i].GroupInstanceId);
            if (err != 0)
                return err;

            err = pd.getKError(Members[i].Err);
            if (err != 0)
                return err;
            int32_t _;
            err = pd.getEmptyTaggedFieldArray(_);
            if (err != 0)
                return err;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t LeaveGroupResponse::key() const
{
    return apiKeyLeaveGroup;
}

int16_t LeaveGroupResponse::version() const
{
    return Version;
}

int16_t LeaveGroupResponse::headerVersion() const
{
    return (Version >= 4) ? 1 : 0;
}

bool LeaveGroupResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool LeaveGroupResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool LeaveGroupResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion LeaveGroupResponse::requiredVersion() const
{
    switch (Version)
    {
    case 4:
        return V2_4_0_0;
    case 3:
        return V2_4_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_9_0_0;
    default:
        return V2_4_0_0;
    }
}

std::chrono::milliseconds LeaveGroupResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}