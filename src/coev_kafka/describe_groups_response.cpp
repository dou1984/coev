#include "describe_groups_response.h"

void DescribeGroupsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DescribeGroupsResponse::encode(PEncoder &pe)
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }
    if (pe.putArrayLength(static_cast<int32_t>(m_groups.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &block : m_groups)
    {
        if (block->encode(pe, m_version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeGroupsResponse::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    if (m_version >= 1)
    {
        if (pd.getDurationMs(m_throttle_time) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    int32_t numGroups;
    if (pd.getArrayLength(numGroups) != ErrNoError)
    {
        return ErrEncodeError;
    }

    m_groups.clear();
    if (numGroups > 0)
    {
        m_groups.resize(numGroups);
        for (int32_t i = 0; i < numGroups; ++i)
        {
            m_groups[i] = std::make_shared<GroupDescription>();
            if (m_groups[i]->decode(pd, m_version) != ErrNoError)
            {
                return ErrEncodeError;
            }
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrEncodeError;
    }
    return ErrNoError;
}

int16_t DescribeGroupsResponse::key() const
{
    return apiKeyDescribeGroups;
}

int16_t DescribeGroupsResponse::version() const
{
    return m_version;
}

int16_t DescribeGroupsResponse::header_version() const
{
    if (m_version >= 5)
    {
        return 1;
    }
    return 0;
}

bool DescribeGroupsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

bool DescribeGroupsResponse::isFlexible() const
{
    return isFlexibleVersion(m_version);
}

bool DescribeGroupsResponse::isFlexibleVersion(int16_t version)
{
    return version >= 5;
}

KafkaVersion DescribeGroupsResponse::required_version() const
{
    switch (m_version)
    {
    case 5:
        return V2_4_0_0;
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
        return V2_4_0_0;
    }
}

std::chrono::milliseconds DescribeGroupsResponse::throttle_time() const
{
    return m_throttle_time;
}

int GroupDescription::encode(PEncoder &pe, int16_t version)
{
    m_version = version;
    pe.putInt16(m_error_code);

    if (pe.putString(m_group_id) != ErrNoError)
        return ErrEncodeError;
    if (pe.putString(m_state) != ErrNoError)
        return ErrEncodeError;
    if (pe.putString(m_protocol_type) != ErrNoError)
        return ErrEncodeError;
    if (pe.putString(m_protocol) != ErrNoError)
        return ErrEncodeError;

    if (pe.putArrayLength(static_cast<int32_t>(m_members.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &pair : m_members)
    {
        if (pair.second->encode(pe, m_version) != ErrNoError)
        {
            return ErrEncodeError;
        }
    }

    if (m_version >= 3)
    {
        pe.putInt32(m_authorized_operations);
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int GroupDescription::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    int16_t error_code;
    if (pd.getInt16(error_code) != ErrNoError)
        return ErrDecodeError;
    m_error_code = static_cast<KError>(error_code);

    if (pd.getString(m_group_id) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_state) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_protocol_type) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_protocol) != ErrNoError)
        return ErrEncodeError;

    int32_t numMembers;
    if (pd.getArrayLength(numMembers) != ErrNoError)
        return ErrDecodeError;

    m_members.clear();
    if (numMembers > 0)
    {
        for (int32_t i = 0; i < numMembers; ++i)
        {
            auto block = std::make_shared<GroupMemberDescription>();
            if (!block->decode(pd, m_version))
            {
                return ErrDecodeError;
            }
            m_members[block->m_member_id] = block;
        }
    }

    if (m_version >= 3)
    {
        if (pd.getInt32(m_authorized_operations) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int GroupMemberDescription::encode(PEncoder &pe, int16_t version)
{
    m_version = version;
    if (pe.putString(m_member_id) != ErrNoError)
        return ErrEncodeError;

    if (version >= 4)
    {
        if (pe.putNullableString(m_group_instance_id) != ErrNoError)
            return ErrEncodeError;
    }

    if (pe.putString(m_client_id) != ErrNoError)
        return ErrEncodeError;
    if (pe.putString(m_client_host) != ErrNoError)
        return ErrEncodeError;
    if (pe.putBytes(m_member_metadata) != ErrNoError)
        return ErrEncodeError;
    if (pe.putBytes(m_member_assignment) != ErrNoError)
        return ErrEncodeError;

    pe.putEmptyTaggedFieldArray();
    return true;
}

int GroupMemberDescription::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    if (pd.getString(m_member_id) != ErrNoError)
        return ErrDecodeError;

    if (version >= 4)
    {
        if (pd.getNullableString(m_group_instance_id) != ErrNoError)
            return ErrDecodeError;
    }

    if (pd.getString(m_client_id) != ErrNoError)
        return ErrDecodeError;
    if (pd.getString(m_client_host) != ErrNoError)
        return ErrDecodeError;
    if (pd.getBytes(m_member_metadata) != ErrNoError)
        return ErrDecodeError;
    if (pd.getBytes(m_member_assignment) != ErrNoError)
        return ErrDecodeError;

    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

std::shared_ptr<ConsumerGroupMemberAssignment> GroupMemberDescription::GetMemberAssignment()
{
    if (m_member_assignment.empty())
    {
        return nullptr;
    }
    auto assignment = std::make_shared<ConsumerGroupMemberAssignment>();
    ::decode(m_member_assignment, std::dynamic_pointer_cast<IDecoder>(assignment), nullptr);
    return assignment;
}

std::shared_ptr<ConsumerGroupMemberMetadata> GroupMemberDescription::GetMemberMetadata()
{
    if (m_member_metadata.empty())
    {
        return nullptr;
    }
    auto metadata = std::make_shared<ConsumerGroupMemberMetadata>();
    ::decode(m_member_metadata, std::dynamic_pointer_cast<IDecoder>(metadata), nullptr);
    return metadata;
}