#include "version.h"
#include "leave_group_response.h"

void LeaveGroupResponse::set_version(int16_t v)
{
    m_version = v;
}

int LeaveGroupResponse::encode(packetEncoder &pe) const
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putKError(m_err);

    if (m_version >= 3)
    {
        int err = pe.putArrayLength(static_cast<int32_t>(m_member_responses.size()));
        if (err != 0)
            return err;

        for (const auto &member : m_member_responses)
        {
            err = pe.putString(member.m_member_id);
            if (err != 0)
                return err;

            err = pe.putNullableString(member.m_group_instance_id);
            if (err != 0)
                return err;

            pe.putKError(member.m_err);
            pe.putEmptyTaggedFieldArray();
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int LeaveGroupResponse::decode(packetDecoder &pd, int16_t version)
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

    if (m_version >= 3)
    {
        int32_t membersLen;
        err = pd.getArrayLength(membersLen);
        if (err != 0)
            return err;

        m_member_responses.resize(membersLen);
        for (int32_t i = 0; i < membersLen; ++i)
        {
            err = pd.getString(m_member_responses[i].m_member_id);
            if (err != 0)
                return err;

            err = pd.getNullableString(m_member_responses[i].m_group_instance_id);
            if (err != 0)
                return err;

            err = pd.getKError(m_member_responses[i].m_err);
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
    return m_version;
}

int16_t LeaveGroupResponse::header_version() const
{
    return (m_version >= 4) ? 1 : 0;
}

bool LeaveGroupResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 4;
}

bool LeaveGroupResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool LeaveGroupResponse::is_flexible_version(int16_t ver) const
{
    return ver >= 4;
}

KafkaVersion LeaveGroupResponse::required_version() const
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

std::chrono::milliseconds LeaveGroupResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}