#include "version.h"
#include "leave_group_request.h"
#include "api_versions.h"

void LeaveGroupRequest::setVersion(int16_t v)
{
    Version = v;
}

int LeaveGroupRequest::encode(PEncoder &pe)
{
    int err = pe.putString(GroupId);
    if (err != 0)
        return err;

    if (Version < 3)
    {
        err = pe.putString(MemberId);
        if (err != 0)
            return err;
    }

    if (Version >= 3)
    {
        err = pe.putArrayLength(static_cast<int32_t>(Members.size()));
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

            pe.putEmptyTaggedFieldArray();
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int LeaveGroupRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getString(GroupId);
    if (err != 0)
        return err;

    if (Version < 3)
    {
        err = pd.getString(MemberId);
        if (err != 0)
            return err;
    }

    if (Version >= 3)
    {
        int32_t memberCount;
        err = pd.getArrayLength(memberCount);
        if (err != 0)
            return err;

        Members.resize(memberCount);
        for (int32_t i = 0; i < memberCount; ++i)
        {
            std::string memberId;
            err = pd.getString(memberId);
            if (err != 0)
                return err;

            std::string instanceId;
            err = pd.getNullableString(instanceId);
            if (err != 0)
                return err;

            Members[i].MemberId = memberId;
            Members[i].GroupInstanceId = instanceId;

            int32_t _;
            err = pd.getEmptyTaggedFieldArray(_);
            if (err != 0)
                return err;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t LeaveGroupRequest::key() const
{
    return apiKeyLeaveGroup;
}

int16_t LeaveGroupRequest::version() const
{
    return Version;
}

int16_t LeaveGroupRequest::headerVersion() const
{
    return (Version >= 4) ? 2 : 1;
}

bool LeaveGroupRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool LeaveGroupRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool LeaveGroupRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion LeaveGroupRequest::requiredVersion() const
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