#include "version.h"
#include "acl_delete_request.h"

void DeleteAclsRequest::set_version(int16_t v)
{
    m_version = v;
}

int DeleteAclsRequest::encode(packet_encoder &pe) const
{
    if (pe.putArrayLength(static_cast<int32_t>(m_filters.size())) != 0)
    {
        return -1;
    }

    for (const auto &filter : m_filters)
    {
        filter->m_version = m_version;
        if (filter->encode(pe) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int DeleteAclsRequest::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;
    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    m_filters.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_filters[i] = std::make_shared<AclFilter>();
        m_filters[i]->m_version = m_version;
        if (m_filters[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int16_t DeleteAclsRequest::key() const
{
    return apiKeyDeleteAcls;
}

int16_t DeleteAclsRequest::version() const
{
    return static_cast<int16_t>(m_version);
}

int16_t DeleteAclsRequest::header_version() const
{
    return 1;
}

bool DeleteAclsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion DeleteAclsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}