#include "version.h"
#include "acl_delete_response.h"

void DeleteAclsResponse::setVersion(int16_t v)
{
    Version = v;
}

int DeleteAclsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(ThrottleTime);

    if (pe.putArrayLength(static_cast<int32_t>(FilterResponses.size())) != 0)
    {
        return -1;
    }

    for (auto &resp : FilterResponses)
    {
        if (resp->encode(pe, Version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int DeleteAclsResponse::decode(PDecoder &pd, int16_t version)
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

    FilterResponses.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        FilterResponses[i] = std::make_shared<FilterResponse>();
        if (FilterResponses[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int16_t DeleteAclsResponse::key() const
{
    return apiKeyDeleteAcls;
}

int16_t DeleteAclsResponse::version() const
{
    return Version;
}

int16_t DeleteAclsResponse::headerVersion() const
{
    return 0;
}

bool DeleteAclsResponse::isValidVersion() const
{
    return Version >= 0 && Version <= 1;
}

KafkaVersion DeleteAclsResponse::requiredVersion() const
{
    switch (Version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds DeleteAclsResponse::throttleTime() const
{
    return ThrottleTime;
}

int MatchingAcl::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(Err);

    if (pe.putNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    if (Resource_.encode(pe, version) != 0)
    {
        return -1;
    }

    if (Acl_.encode(pe) != 0)
    {
        return -1;
    }

    return 0;
}

int MatchingAcl::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(Err) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    if (Resource_.decode(pd, version) != 0)
    {
        return -1;
    }

    if (Acl_.decode(pd, version) != 0)
    {
        return -1;
    }

    return 0;
}

int FilterResponse::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(Err);

    if (pe.putNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(MatchingAcls.size())) != 0)
    {
        return -1;
    }

    for (auto &acl : MatchingAcls)
    {
        if (acl->encode(pe, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int FilterResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(Err) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(ErrMsg) != 0)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    MatchingAcls.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        MatchingAcls[i] = std::make_shared<MatchingAcl>();
        if (MatchingAcls[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}