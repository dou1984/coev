#include "version.h"
#include "acl_create_response.h"

void CreateAclsResponse::set_version(int16_t v)
{
    m_version = v;
}

int CreateAclsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_acl_creation_responses.size())) != 0)
    {
        return -1;
    }

    for (auto &resp : m_acl_creation_responses)
    {
        if (resp->encode(pe) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int CreateAclsResponse::decode(packet_decoder &pd, int16_t version)
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

    m_acl_creation_responses.resize(n);
    for (int32_t i = 0; i < n; ++i)
    {
        m_acl_creation_responses[i] = std::make_shared<AclCreationResponse>();
        if (m_acl_creation_responses[i]->decode(pd, version) != 0)
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
    return m_version;
}

int16_t CreateAclsResponse::header_version() const
{
    return 0;
}

bool CreateAclsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 1;
}

KafkaVersion CreateAclsResponse::required_version() const
{
    switch (m_version)
    {
    case 1:
        return V2_0_0_0;
    default:
        return V0_11_0_0;
    }
}

std::chrono::milliseconds CreateAclsResponse::throttle_time() const
{
    return m_throttle_time;
}

int AclCreationResponse::encode(packet_encoder &pe) const
{
    pe.putKError(m_err);

    // Always write nullable string to match decode method
    if (pe.putNullableString(m_err_msg) != 0)
    {
        return -1;
    }

    return 0;
}

int AclCreationResponse::decode(packet_decoder &pd, int16_t version)
{
    if (pd.getKError(m_err) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(m_err_msg) != 0)
    {
        return -1;
    }

    return 0;
}