#include "version.h"
#include "acl_create_request.h"

void CreateAclsRequest::setVersion(int16_t v)
{
    Version = v;
}

int CreateAclsRequest::encode(PEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(AclCreations.size())) != 0)
    {
        return -1;
    }

    for (auto &aclCreation : AclCreations)
    {
        if (aclCreation->encode(pe, Version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int CreateAclsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    AclCreations.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        AclCreations[i] = std::make_shared<AclCreation>();
        if (AclCreations[i]->decode(pd, version) != 0)
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
    return Version;
}

int16_t CreateAclsRequest::headerVersion() const
{
    return 1;
}

bool CreateAclsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion CreateAclsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

int AclCreation::encode(PEncoder &pe, int16_t version)
{
    if (resource.encode(pe, version) != 0)
    {
        return -1;
    }
    if (acl.encode(pe) != 0)
    {
        return -1;
    }
    return 0;
}

int AclCreation::decode(PDecoder &pd, int16_t version)
{
    if (resource.decode(pd, version) != 0)
    {
        return -1;
    }
    if (acl.decode(pd, version) != 0)
    {
        return -1;
    }
    return 0;
}