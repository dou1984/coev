#include "version.h"
#include "api_versions.h"
#include "delete_groups_response.h"

void DeleteGroupsResponse::set_version(int16_t v)
{
    m_version = v;
}

int DeleteGroupsResponse::encode(packet_encoder &pe) const
{
    pe.putDurationMs(m_throttle_time);

    if (pe.putArrayLength(static_cast<int32_t>(m_group_error_codes.size())) != ErrNoError)
    {
        return ErrEncodeError;
    }

    for (auto &kv : m_group_error_codes)
    {
        if (pe.putString(kv.first) != ErrNoError)
        {
            return ErrEncodeError;
        }
        pe.putInt16(static_cast<int16_t>(kv.second));
        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return ErrNoError;
}

int DeleteGroupsResponse::decode(packet_decoder &pd, int16_t version)
{
    m_version = version;

    if (pd.getDurationMs(m_throttle_time) != ErrNoError)
    {
        return ErrDecodeError;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return ErrDecodeError;
    }

    m_group_error_codes.clear();

    if (n > 0)
    {
        for (int32_t i = 0; i < n; ++i)
        {
            std::string groupID;
            if (pd.getString(groupID) != ErrNoError)
            {
                return ErrDecodeError;
            }

            int16_t errCode;
            if (pd.getInt16(errCode) != ErrNoError)
            {
                return ErrDecodeError;
            }
            m_group_error_codes[groupID] = static_cast<KError>(errCode);
            int32_t _;
            if (pd.getEmptyTaggedFieldArray(_) != ErrNoError)
            {
                return ErrDecodeError;
            }
        }
    }

    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t DeleteGroupsResponse::key() const
{
    return apiKeyDeleteGroups;
}

int16_t DeleteGroupsResponse::version() const
{
    return m_version;
}

int16_t DeleteGroupsResponse::header_version() const
{
    return m_version >= 2 ? 1 : 0;
}

bool DeleteGroupsResponse::is_flexible() const
{
    return is_flexible_version(m_version);
}

bool DeleteGroupsResponse::is_flexible_version(int16_t version) const
{
    return version >= 2;
}

bool DeleteGroupsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 2;
}

KafkaVersion DeleteGroupsResponse::required_version() const
{
    switch (m_version)
    {
    case 2:
        return V2_4_0_0;
    case 1:
        return V2_0_0_0;
    case 0:
        return V1_1_0_0;
    default:
        return V2_0_0_0;
    }
}

std::chrono::milliseconds DeleteGroupsResponse::throttle_time() const
{
    return m_throttle_time;
}