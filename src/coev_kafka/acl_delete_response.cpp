#include "version.h"
#include "acl_delete_response.h"

void DeleteAclsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DeleteAclsResponse::encode(PEncoder &pe)
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_filter_responses.size())) != 0)
    {
        return -1;
    }

    for (auto &resp : m_filter_responses)
    {
        if (resp->encode(pe, m_version) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int DeleteAclsResponse::decode(PDecoder &pd, int16_t version)
{
    if (pd.getDurationMs(m_throttle_time) != 0)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    m_filter_responses.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_filter_responses[i] = std::make_shared<FilterResponse>();
        if (m_filter_responses[i]->decode(pd, version) != 0)
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
    return m_version;
}

int16_t DeleteAclsResponse::header_version() const
{
    return 0;
}

bool DeleteAclsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion DeleteAclsResponse::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds DeleteAclsResponse::throttle_time() const
{
    return m_throttle_time;
}

int MatchingAcl::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(m_err);

    if (pe.putNullableString(m_err_msg) != 0)
    {
        return -1;
    }

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

int MatchingAcl::decode(PDecoder &pd, int16_t version)
{
    if (pd.getKError(m_err) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(m_err_msg) != 0)
    {
        return -1;
    }

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

int FilterResponse::encode(PEncoder &pe, int16_t version)
{
    pe.putKError(m_err);

    if (pe.putNullableString(m_err_msg) != 0)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_matching_acls.size())) != 0)
    {
        return -1;
    }

    for (auto &acl : m_matching_acls)
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
    if (pd.getKError(m_err) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(m_err_msg) != 0)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    m_matching_acls.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_matching_acls[i] = std::make_shared<MatchingAcl>();
        if (m_matching_acls[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}