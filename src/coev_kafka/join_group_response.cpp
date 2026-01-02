#include "version.h"
#include "join_group_response.h"
#include "api_versions.h"

void JoinGroupResponse::setVersion(int16_t v)
{
    Version = v;
}

int JoinGroupResponse::encode(PEncoder &pe)
{
    if (Version >= 2)
    {
        pe.putDurationMs(ThrottleTime);
    }

    pe.putKError(Err);
    pe.putInt32(GenerationId);

    int err = pe.putString(GroupProtocol);
    if (err != 0)
        return err;

    err = pe.putString(LeaderId);
    if (err != 0)
        return err;

    err = pe.putString(MemberId);
    if (err != 0)
        return err;

    err = pe.putArrayLength(static_cast<int32_t>(Members.size()));
    if (err != 0)
        return err;

    for (auto &member : Members)
    {
        err = pe.putString(member.MemberId);
        if (err != 0)
            return err;

        if (Version >= 5)
        {
            err = pe.putNullableString(member.GroupInstanceId);
            if (err != 0)
                return err;
        }

        err = pe.putBytes(member.Metadata);
        if (err != 0)
            return err;

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int JoinGroupResponse::decode(PDecoder &pd, int16_t version)
{
    Version = version;

    if (version >= 2)
    {
        int err = pd.getDurationMs(ThrottleTime);
        if (err != 0)
            return err;
    }

    int err = pd.getKError(Err);
    if (err != 0)
        return err;

    err = pd.getInt32(GenerationId);
    if (err != 0)
        return err;

    err = pd.getString(GroupProtocol);
    if (err != 0)
        return err;

    err = pd.getString(LeaderId);
    if (err != 0)
        return err;

    err = pd.getString(MemberId);
    if (err != 0)
        return err;

    int32_t n;
    err = pd.getArrayLength(n);
    if (err != 0)
        return err;

    if (n == 0)
    {
        int32_t _;
        return pd.getEmptyTaggedFieldArray(_);
    }

    Members.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        std::string memberId;
        err = pd.getString(memberId);
        if (err != 0)
            return err;

        std::string groupInstanceId;
        if (Version >= 5)
        {
            err = pd.getNullableString(groupInstanceId);
            if (err != 0)
                return err;
        }

        std::string metadata;
        err = pd.getBytes(metadata);
        if (err != 0)
            return err;

        Members[i] = {memberId, groupInstanceId, metadata};
        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
            return err;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t JoinGroupResponse::key() const
{
    return apiKeyJoinGroup;
}

int16_t JoinGroupResponse::version() const
{
    return Version;
}

int16_t JoinGroupResponse::headerVersion() const
{
    return (Version >= 6) ? 1 : 0;
}

bool JoinGroupResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 6;
}

bool JoinGroupResponse::isFlexible()
{
    return isFlexibleVersion(Version);
}

bool JoinGroupResponse::isFlexibleVersion(int16_t ver)
{
    return ver >= 6;
}

KafkaVersion JoinGroupResponse::requiredVersion() const
{
    switch (Version)
    {
    case 6:
        return V2_4_0_0;
    case 5:
        return V2_3_0_0;
    case 4:
        return V2_2_0_0;
    case 3:
        return V2_0_0_0;
    case 2:
        return V0_11_0_0;
    case 1:
        return V0_10_1_0;
    case 0:
        return V0_10_0_0;
    default:
        return V2_3_0_0;
    }
}

std::chrono::milliseconds JoinGroupResponse::throttleTime() const
{
    return std::chrono::milliseconds(ThrottleTime);
}

int JoinGroupResponse::GetMembers(std::map<std::string, ConsumerGroupMemberMetadata> &members_)
{
    members_.clear();

    for (auto &member : Members)
    {
        auto meta = std::make_shared<ConsumerGroupMemberMetadata>();
        int err = ::decode(member.Metadata, std::dynamic_pointer_cast<IDecoder>(meta), nullptr);
        if (err != 0)
            return err;
        // members_[member.MemberId] = meta;
    }
    return 0;
}