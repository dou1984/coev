#include "version.h"
#include "acl_describe_response.h"

void DescribeAclsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DescribeAclsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);
    pe.putKError(m_err);

    if (pe.putNullableString(m_err_msg) != 0)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_resource_acls.size())) != 0)
    {
        return -1;
    }

    for (auto &resourceAcl : m_resource_acls)
    {
        if (resourceAcl->encode(pe, m_version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int DescribeAclsResponse::decode(packet_decoder &pd, int16_t version)
{
    if (pd.getDurationMs(m_throttle_time) != 0)
    {
        return -1;
    }

    if (pd.getKError(m_err) != 0)
    {
        return -1;
    }

    std::string errmsg;
    if (pd.getString(errmsg) != 0)
    {
        return -1;
    }
    if (!errmsg.empty())
    {
        m_err_msg = std::move(errmsg);
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    m_resource_acls.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_resource_acls[i] = std::make_shared<ResourceAcls>();
        if (m_resource_acls[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int16_t DescribeAclsResponse::key() const
{
    return apiKeyDescribeAcls;
}

int16_t DescribeAclsResponse::version() const
{
    return m_version;
}

int16_t DescribeAclsResponse::header_version() const
{
    return 0;
}

bool DescribeAclsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion DescribeAclsResponse::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds DescribeAclsResponse::throttle_time() const
{
    return m_throttle_time;
}