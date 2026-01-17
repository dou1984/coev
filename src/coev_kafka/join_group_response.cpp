#include "version.h"
#include "join_group_response.h"
#include "api_versions.h"

void JoinGroupResponse::set_version(int16_t v)
{
    m_version = v;
}

int JoinGroupResponse::encode(packetEncoder &pe)
{
    if (m_version >= 2)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putKError(m_err);
    pe.putInt32(m_generation_id);

    int err = pe.putString(m_group_protocol);
    if (err != 0)
        return err;

    err = pe.putString(m_leader_id);
    if (err != 0)
        return err;

    err = pe.putString(m_member_id);
    if (err != 0)
        return err;

    err = pe.putArrayLength(static_cast<int32_t>(m_members.size()));
    if (err != 0)
        return err;

    for (auto &member : m_members)
    {
        err = pe.putString(member.m_member_id);
        if (err != 0)
            return err;

        if (m_version >= 5)
        {
            err = pe.putNullableString(member.m_group_instance_id);
            if (err != 0)
                return err;
        }

        err = pe.putBytes(member.m_metadata);
        if (err != 0)
            return err;

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int JoinGroupResponse::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;

    if (version >= 2)
    {
        int err = pd.getDurationMs(m_throttle_time);
        if (err != 0)
            return err;
    }

    int err = pd.getKError(m_err);
    if (err != 0)
        return err;

    err = pd.getInt32(m_generation_id);
    if (err != 0)
        return err;

    err = pd.getString(m_group_protocol);
    if (err != 0)
        return err;

    err = pd.getString(m_leader_id);
    if (err != 0)
        return err;

    err = pd.getString(m_member_id);
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

    m_members.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        std::string memberId;
        err = pd.getString(memberId);
        if (err != 0)
            return err;

        std::string groupInstanceId;
        if (m_version >= 5)
        {
            err = pd.getNullableString(groupInstanceId);
            if (err != 0)
                return err;
        }

        std::string metadata;
        err = pd.getBytes(metadata);
        if (err != 0)
            return err;

        m_members[i] = {memberId, groupInstanceId, metadata};
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
    return m_version;
}

int16_t JoinGroupResponse::header_version() const
{
    return (m_version >= 6) ? 1 : 0;
}

bool JoinGroupResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 6;
}

bool JoinGroupResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool JoinGroupResponse::is_flexible_version(int16_t ver) const
{
    return ver >= 6;
}

KafkaVersion JoinGroupResponse::required_version() const
{
    switch (m_version)
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

std::chrono::milliseconds JoinGroupResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}

int JoinGroupResponse::GetMembers(std::map<std::string, ConsumerGroupMemberMetadata> &members_)
{
    members_.clear();

    for (auto &member : m_members)
    {
        auto& meta =members_[member.m_member_id];
        int err = ::decode(member.m_metadata, meta);
        if (err != 0)
            return err;
    }
    return 0;
}