#include "version.h"
#include "describe_groups_request.h"

void DescribeGroupsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DescribeGroupsRequest::encode(packetEncoder &pe) const
{
    if (pe.putStringArray(m_groups) != ErrNoError)
    {
        return ErrEncodeError;
    }
    if (m_version >= 3)
    {
        pe.putBool(m_include_authorized_operations);
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DescribeGroupsRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    if (pd.getStringArray(m_groups) != ErrNoError)
    {
        return ErrDecodeError;
    }
    if (m_version >= 3)
    {
        bool val;
        if (pd.getBool(val) != ErrNoError)
        {
            return ErrDecodeError;
        }
        m_include_authorized_operations = val;
    }
    int32_t dummy;
    if (pd.getEmptyTaggedFieldArray(dummy) != ErrNoError)
    {
        return ErrDecodeError;
    }
    return ErrNoError;
}

int16_t DescribeGroupsRequest::key() const
{
    return apiKeyDescribeGroups;
}

int16_t DescribeGroupsRequest::version() const
{
    return m_version;
}

int16_t DescribeGroupsRequest::header_version() const
{
    if (m_version >= 5)
    {
        return 2;
    }
    return 1;
}

bool DescribeGroupsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

bool DescribeGroupsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DescribeGroupsRequest::is_flexible_version(int16_t version) const
{
    return version >= 5;
}

KafkaVersion DescribeGroupsRequest::required_version() const
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

void DescribeGroupsRequest::add_group(const std::string &group)
{
    m_groups.push_back(group);
}