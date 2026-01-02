#include "version.h"
#include "acl_describe_response.h"

void DescribeAclsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DescribeAclsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);
    pe.putKError(Err);

    if (pe.putNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(ResourceAcls_.size())) != 0)
    {
        return -1;
    }

    for (auto &resourceAcl : ResourceAcls_)
    {
        if (resourceAcl->encode(pe, Version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int DescribeAclsResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(ThrottleTime) != 0)
    {
        return -1;
    }

    if (pd.getKError(Err) != 0)
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
        ErrMsg = std::move(errmsg);
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    ResourceAcls_.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        ResourceAcls_[i] = std::make_shared<ResourceAcls>();
        if (ResourceAcls_[i]->decode(pd, version) != 0)
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
    return Version;
}

int16_t DescribeAclsResponse::headerVersion() const
{
    return 0;
}

bool DescribeAclsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion DescribeAclsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds DescribeAclsResponse::throttleTime() const
{
    return ThrottleTime;
}