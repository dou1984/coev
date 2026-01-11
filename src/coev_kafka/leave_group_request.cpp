#include "version.h"
#include "leave_group_request.h"
#include "api_versions.h"

void LeaveGroupRequest::set_version(int16_t v)
{
    m_version = v;
}

int LeaveGroupRequest::encode(PEncoder &pe)
{
    int err = pe.putString(m_group_id);
    if (err != 0)
        return err;

    if (m_version < 3)
    {
        err = pe.putString(m_member_id);
        if (err != 0)
            return err;
    }

    if (m_version >= 3)
    {
        err = pe.putArrayLength(static_cast<int32_t>(m_members.size()));
        if (err != 0)
            return err;

        for (auto &member : m_members)
        {
            err = pe.putString(member.m_member_id);
            if (err != 0)
                return err;

            err = pe.putNullableString(member.m_group_instance_id);
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
    m_version = version;

    int err = pd.getString(m_group_id);
    if (err != 0)
        return err;

    if (m_version < 3)
    {
        err = pd.getString(m_member_id);
        if (err != 0)
            return err;
    }

    if (m_version >= 3)
    {
        int32_t memberCount;
        err = pd.getArrayLength(memberCount);
        if (err != 0)
            return err;

        m_members.resize(memberCount);
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

            m_members[i].m_member_id = memberId;
            m_members[i].m_group_instance_id = instanceId;

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
    return m_version;
}

int16_t LeaveGroupRequest::header_version() const
{
    return (m_version >= 4) ? 2 : 1;
}

bool LeaveGroupRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool LeaveGroupRequest::isFlexible()
{
    return isFlexibleVersion(m_version);
}

bool LeaveGroupRequest::isFlexibleVersion(int16_t ver)
{
    return ver >= 4;
}

KafkaVersion LeaveGroupRequest::required_version() const
{
    switch (m_version)
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