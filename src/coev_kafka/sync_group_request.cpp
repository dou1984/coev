#include "version.h"
#include "sync_group_request.h"
#include "api_versions.h"
#include <utility>

int SyncGroupRequestAssignment::encode(packet_encoder &pe, int16_t version) const
{
    int err = pe.putString(m_member_id);
    if (err != 0)
        return err;

    err = pe.putBytes(m_assignment);
    if (err != 0)
        return err;

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int SyncGroupRequestAssignment::decode(packet_decoder &pd, int16_t version)
{
    int err = pd.getString(m_member_id);
    if (err != 0)
        return err;

    err = pd.getBytes(m_assignment);
    if (err != 0)
        return err;

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_); // consume tagged fields if any
}

// --- SyncGroupRequest ---

void SyncGroupRequest::set_version(int16_t v)
{
    m_version = v;
}

int SyncGroupRequest::encode(packet_encoder &pe) const
{
    int err = pe.putString(m_group_id);
    if (err != 0)
        return err;

    pe.putInt32(m_generation_id);

    err = pe.putString(m_member_id);
    if (err != 0)
        return err;

    if (m_version >= 3)
    {
        err = pe.putNullableString(m_group_instance_id);
        if (err != 0)
            return err;
    }

    err = pe.putArrayLength(static_cast<int>(m_group_assignments.size()));
    if (err != 0)
        return err;

    for (auto &assignment : m_group_assignments)
    {
        err = assignment.encode(pe, m_version);
        if (err != 0)
            return err;
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int SyncGroupRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    int err = pd.getString(m_group_id);
    if (err != 0)
        return err;

    err = pd.getInt32(m_generation_id);
    if (err != 0)
        return err;

    err = pd.getString(m_member_id);
    if (err != 0)
        return err;

    if (m_version >= 3)
    {

        err = pd.getNullableString(m_group_instance_id);
        if (err != 0)
            return err;
    }

    int numAssignments = 0;
    err = pd.getArrayLength(numAssignments);
    if (err != 0)
        return err;

    m_group_assignments.clear();
    if (numAssignments > 0)
    {
        m_group_assignments.reserve(numAssignments);
        for (int i = 0; i < numAssignments; ++i)
        {
            SyncGroupRequestAssignment block;
            err = block.decode(pd, m_version);
            if (err != 0)
                return err;
            m_group_assignments.push_back(std::move(block));
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
    return m_version;
}

int16_t SyncGroupRequest::header_version() const
{
    return (m_version >= 4) ? 2 : 1;
}

bool SyncGroupRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

bool SyncGroupRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool SyncGroupRequest::is_flexible_version(int16_t ver) const
{
    return ver >= 4;
}

KafkaVersion SyncGroupRequest::required_version() const
{
    switch (m_version)
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
    m_group_assignments.emplace_back(memberId, memberAssignment);
}

int SyncGroupRequest::AddGroupAssignmentMember(const std::string &memberId, std::shared_ptr<ConsumerGroupMemberAssignment> memberAssignment)
{
    std::string bin;
    int err = ::encode(*memberAssignment, bin);
    if (err)
        return err;
    AddGroupAssignment(memberId, bin);
    return 0;
}