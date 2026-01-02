#include "version.h"
#include "acl_describe_request.h"

void DescribeAclsRequest::setVersion(int16_t v)
{
    Version = static_cast<int>(v);
    Filter.Version = Version;
}

int DescribeAclsRequest::encode(PEncoder &pe)
{
    return Filter.encode(pe);
}

int DescribeAclsRequest::decode(PDecoder &pd, int16_t version)
{
    Version = static_cast<int>(version);
    Filter.Version = Version;
    return Filter.decode(pd, version);
}

int16_t DescribeAclsRequest::key() const
{
    return apiKeyDescribeAcls;
}

int16_t DescribeAclsRequest::version() const
{
    return static_cast<int16_t>(Version);
}

int16_t DescribeAclsRequest::headerVersion() const
{
    return 1;
}

bool DescribeAclsRequest::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion DescribeAclsRequest::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}