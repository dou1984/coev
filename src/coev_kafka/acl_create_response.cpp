#include "version.h"
#include "acl_create_response.h"

void CreateAclsResponse::setVersion(int16_t v)
{
    Version = v;
}

int CreateAclsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (pe.putArrayLength(static_cast<int32_t>(AclCreationResponses_.size())) != 0)
    {
        return -1;
    }

    for (auto &resp : AclCreationResponses_)
    {
        if (resp->encode(pe) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int CreateAclsResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(ThrottleTime) != 0)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    AclCreationResponses_.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        AclCreationResponses_[i] = std::make_shared<AclCreationResponse>();
        if (AclCreationResponses_[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int16_t CreateAclsResponse::key() const
{
    return apiKeyCreateAcls;
}

int16_t CreateAclsResponse::version() const
{
    return Version;
}

int16_t CreateAclsResponse::headerVersion() const
{
    return 0;
}

bool CreateAclsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion CreateAclsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds CreateAclsResponse::throttleTime() const
{
    return ThrottleTime;
}

int AclCreationResponse::encode(PEncoder &pe)
{
    pe.putKError(Err);

    if (pe.putNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    return 0;
}

int AclCreationResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(Err) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    return 0;
}