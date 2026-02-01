#include "version.h"
#include "sync_group_response.h"
#include "api_versions.h"

void SyncGroupResponse::set_version(int16_t v)
{
    m_version = v;
}

int SyncGroupResponse::GetMemberAssignment(std::shared_ptr<ConsumerGroupMemberAssignment> &assignment)
{
    assignment = std::make_shared<ConsumerGroupMemberAssignment>();
    return ::decode(m_member_assignment, *assignment);
}

int SyncGroupResponse::encode(packet_encoder &pe) const
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }
    pe.putKError(m_err);
    int err = pe.putBytes(m_member_assignment);
    if (err != 0)
    {
        return err;
    }
    pe.putEmptyTaggedFieldArray();
    return 0;
}

int SyncGroupResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    int err = 0;
    if (m_version >= 1)
    {

        err = pd.getDurationMs(m_throttle_time);
        if (err != 0)
        {
            return err;
        }
    }
    err = pd.getKError(m_err);
    if (err != 0)
    {
        return err;
    }
    err = pd.getBytes(m_member_assignment);
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
    return m_version;
}

int16_t SyncGroupResponse::header_version() const
{
    if (m_version >= 4)
    {
        return 1;
    }
    return 0;
}

bool SyncGroupResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

bool SyncGroupResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool SyncGroupResponse::is_flexible_version(int16_t ver) const
{
    return ver >= 4;
}

KafkaVersion SyncGroupResponse::required_version() const
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

std::chrono::milliseconds SyncGroupResponse::throttle_time() const
{
    return std::chrono::milliseconds(m_throttle_time);
}