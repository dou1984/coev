#include "version.h"
#include "api_versions.h"
#include "delete_groups_request.h"

void DeleteGroupsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DeleteGroupsRequest::encode(PEncoder &pe)
{
    if (pe.putStringArray(m_groups) != ErrNoError)
    {
        return ErrEncodeError;
    }
    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DeleteGroupsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = version;
    if (pd.getStringArray(m_groups) != ErrNoError)
    {
        return ErrDecodeError;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t DeleteGroupsRequest::key() const
{
    return apiKeyDeleteGroups;
}

int16_t DeleteGroupsRequest::version() const
{
    return m_version;
}

int16_t DeleteGroupsRequest::header_version() const
{
    return m_version >= 2 ? 2 : 1;
}

bool DeleteGroupsRequest::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DeleteGroupsRequest::is_flexible_version(int16_t version)
{
    return version >= 2;
}

bool DeleteGroupsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion DeleteGroupsRequest::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_4_0_0;
    case 1:
        return V2_0_0_0;
    case 0:
        return V1_1_0_0;
    default:
        return V2_0_0_0;
    }
}

void DeleteGroupsRequest::AddGroup(const std::string &group)
{
    m_groups.push_back(group);
}