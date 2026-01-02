#include "version.h"
#include "sync_group_request.h"
#include "api_versions.h"
#include <utility>

int SyncGroupRequestAssignment::encode(PEncoder &pe, int16_t version)
{
    int err = pe.putString(MemberId);
    if (err != 0)
        return err;

    err = pe.putBytes(Assignment);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int SyncGroupRequestAssignment::decode(PDecoder &pd, int16_t version)
{
    int err = pd.getString(MemberId);
    if (err != 0)
        return err;

    err = pd.getBytes(Assignment);
    if (err != 0)
        return err;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_); // consume tagged fields if any
}

// --- SyncGroupRequest ---

void SyncGroupRequest::setVersion(int16_t v)
{
    Version = v;
}

int SyncGroupRequest::encode(PEncoder &pe)
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
            return err;
    }

    err = pe.putArrayLength(static_cast<int>(GroupAssignments.size()));
    if (err != 0)
        return err;

    for (auto &assignment : GroupAssignments)
    {
        err = assignment.encode(pe, Version);
        if (err != 0)
            return err;
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int SyncGroupRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    int err = pd.getString(GroupId);
    if (err != 0)
        return err;

    err = pd.getInt32(GenerationId);
    if (err != 0)
        return err;

    err = pd.getString(MemberId);
    if (err != 0)
        return err;

    if (Version >= 3)
    {

        err = pd.getNullableString(GroupInstanceId);
        if (err != 0)
            return err;
    }

    int numAssignments = 0;
    err = pd.getArrayLength(numAssignments);
    if (err != 0)
        return err;

    GroupAssignments.clear();
    if (numAssignments > 0)
    {
        GroupAssignments.reserve(numAssignments);
        for (int i = 0; i < numAssignments; ++i)
        {
            SyncGroupRequestAssignment block;
            err = block.decode(pd, Version);
            if (err != 0)
                return err;
            GroupAssignments.push_back(std::move(block));
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t SyncGroupRequest::key() const
{
    return apiKeySyncGroup;
}

int16_t SyncGroupRequest::version() const
{
    return Version;
}

int16_t SyncGroupRequest::headerVersion() const
{
    return (Version >= 4) ? 2 : 1;
}

bool SyncGroupRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 4;
}

bool SyncGroupRequest::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool SyncGroupRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion SyncGroupRequest::requiredVersion() const
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

void SyncGroupRequest::AddGroupAssignment(const std::string &memberId,
                                          const std::string &memberAssignment)
{
    GroupAssignments.emplace_back(memberId, memberAssignment);
}

int SyncGroupRequest::AddGroupAssignmentMember(const std::string &memberId, std::shared_ptr<ConsumerGroupMemberAssignment> memberAssignment)
{
    std::string bin;
    int err = ::encode(memberAssignment, bin, nullptr);
    if (err)
        return err;
    AddGroupAssignment(memberId, bin);
    return 0;
}