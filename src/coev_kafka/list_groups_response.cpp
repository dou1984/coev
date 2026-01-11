#include "version.h"
#include "list_groups_response.h"
#include <vector>
#include "api_versions.h"

void ListGroupsResponse::set_version(int16_t v)
{
    m_version = v;
}

int ListGroupsResponse::encode(PEncoder &pe)
{
    if (m_version >= 1)
    {
        pe.putDurationMs(m_throttle_time);
    }

    pe.putKError(m_err);

    int err = pe.putArrayLength(static_cast<int32_t>(m_groups.size()));
    if (err != 0)
        return err;

    for (auto &kv : m_groups)
    {
        const std::string &groupId = kv.first;
        const std::string &protocolType = kv.second;

        err = pe.putString(groupId);
        if (err != 0)
            return err;

        err = pe.putString(protocolType);
        if (err != 0)
            return err;

        if (m_version >= 4)
        {
            const GroupData &gd = m_groups_data[groupId]; // safe: assumed populated
            err = pe.putString(gd.m_group_state);
            if (err != 0)
                return err;
        }

        if (m_version >= 5)
        {
            const GroupData &gd = m_groups_data[groupId];
            err = pe.putString(gd.m_group_type);
            if (err != 0)
                return err;
        }

        pe.putEmptyTaggedFieldArray();
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListGroupsResponse::decode(PDecoder &pd, int16_t version)
{
    m_version = version;

    if (m_version >= 1)
    {
        int err = pd.getDurationMs(m_throttle_time);
        if (err != 0)
            return err;
    }

    int err = pd.getKError(m_err);
    if (err != 0)
        return err;

    int32_t n;
    err = pd.getArrayLength(n);
    if (err != 0)
        return err;

    if (n > 0)
    {
        m_groups.clear();
        m_groups.reserve(n);
        if (m_version >= 4)
        {
            m_groups_data.clear();
            m_groups_data.reserve(n);
        }
    }

    for (int32_t i = 0; i < n; ++i)
    {
        std::string groupId, protocolType;
        err = pd.getString(groupId);
        if (err != 0)
            return err;

        err = pd.getString(protocolType);
        if (err != 0)
            return err;

        m_groups[groupId] = protocolType;

        if (m_version >= 4)
        {
            GroupData gd;
            err = pd.getString(gd.m_group_state);
            if (err != 0)
                return err;

            if (m_version >= 5)
            {
                err = pd.getString(gd.m_group_type);
                if (err != 0)
                    return err;
            }

            m_groups_data[groupId] = std::move(gd);
        }
        int32_t _;
        err = pd.getEmptyTaggedFieldArray(_);
        if (err != 0)
            return err;
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ListGroupsResponse::key() const
{
    return apiKeyListGroups;
}

int16_t ListGroupsResponse::version() const
{
    return m_version;
}

int16_t ListGroupsResponse::header_version() const
{
    return (m_version >= 3) ? 1 : 0;
}

bool ListGroupsResponse::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

bool ListGroupsResponse::is_flexible()
{
    return is_flexible_version(m_version);
}

bool ListGroupsResponse::is_flexible_version(int16_t ver)
{
    return ver >= 3;
}

KafkaVersion ListGroupsResponse::required_version() const
{
    switch (m_version)
    {
    case 5:
        return V3_8_0_0;
    case 4:
        return V2_6_0_0;
    case 3:
        return V2_4_0_0;
    case 2:
        return V2_0_0_0;
    case 1:
        return V0_11_0_0;
    case 0:
        return V0_9_0_0;
    default:
        return V2_6_0_0;
    }
}