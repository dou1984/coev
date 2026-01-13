#include "version.h"
#include "acl_create_request.h"

void CreateAclsRequest::set_version(int16_t v)
{
    m_version = v;
}

int CreateAclsRequest::encode(packetEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(m_acl_creations.size())) != 0)
    {
        return -1;
    }

    for (auto &aclCreation : m_acl_creations)
    {
        if (aclCreation->encode(pe, m_version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int CreateAclsRequest::decode(packetDecoder &pd, int16_t version)
{
    m_version = version;
    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    m_acl_creations.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_acl_creations[i] = std::make_shared<AclCreation>();
        if (m_acl_creations[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int16_t CreateAclsRequest::key() const
{
    return apiKeyCreateAcls;
}

int16_t CreateAclsRequest::version() const
{
    return m_version;
}

int16_t CreateAclsRequest::header_version() const
{
    return 1;
}

bool CreateAclsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion CreateAclsRequest::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

int AclCreation::encode(packetEncoder &pe, int16_t version)
{
    if (m_resource.encode(pe, version) != 0)
    {
        return -1;
    }
    if (m_acl.encode(pe) != 0)
    {
        return -1;
    }
    return 0;
}

int AclCreation::decode(packetDecoder &pd, int16_t version)
{
    if (m_resource.decode(pd, version) != 0)
    {
        return -1;
    }
    if (m_acl.decode(pd, version) != 0)
    {
        return -1;
    }
    return 0;
}