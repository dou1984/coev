#include "version.h"
#include "acl_delete_request.h"

void DeleteAclsRequest::setVersion(int16_t v)
{
    Version = v;
}

int DeleteAclsRequest::encode(PEncoder &pe)
{
    if (pe.putArrayLength(static_cast<int32_t>(Filters.size())) != 0)
    {
        return -1;
    }

    for (auto &filter : Filters)
    {
        filter->Version = Version;
        if (filter->encode(pe) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int DeleteAclsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = version;
    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    Filters.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        Filters[i] = std::make_shared<AclFilter>();
        Filters[i]->Version = Version;
        if (Filters[i]->decode(pd, version) != 0)
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
    return static_cast<int16_t>(Version);
}

int16_t DeleteAclsRequest::headerVersion() const
{
    return 1;
}

bool DeleteAclsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion DeleteAclsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}