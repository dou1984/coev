#include "version.h"
#include "acl_describe_request.h"

void DescribeAclsRequest::set_version(int16_t v)
{
    m_version = static_cast<int>(v);
    m_filter.m_version = m_version;
}

int DescribeAclsRequest::encode(PEncoder &pe)
{
    return m_filter.encode(pe);
}

int DescribeAclsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = static_cast<int>(version);
    m_filter.m_version = m_version;
    return m_filter.decode(pd, version);
}

int16_t DescribeAclsRequest::key() const
{
    return apiKeyDescribeAcls;
}

int16_t DescribeAclsRequest::version() const
{
    return static_cast<int16_t>(m_version);
}

int16_t DescribeAclsRequest::headerVersion() const
{
    return 1;
}

bool DescribeAclsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion DescribeAclsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}