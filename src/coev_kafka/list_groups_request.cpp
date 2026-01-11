#include "version.h"
#include "list_groups_request.h"
#include "api_versions.h"

void ListGroupsRequest::set_version(int16_t v)
{
    m_version = v;
}

int ListGroupsRequest::encode(PEncoder &pe)
{
    if (m_version >= 4)
    {
        int err = pe.putArrayLength(static_cast<int32_t>(m_states_filter.size()));
        if (err != 0)
            return err;

        for (auto &filter : m_states_filter)
        {
            err = pe.putString(filter);
            if (err != 0)
                return err;
        }

        if (m_version >= 5)
        {
            err = pe.putArrayLength(static_cast<int32_t>(m_types_filter.size()));
            if (err != 0)
                return err;

            for (auto &filter : m_types_filter)
            {
                err = pe.putString(filter);
                if (err != 0)
                    return err;
            }
        }
    }

    pe.putEmptyTaggedFieldArray();
    return 0;
}

int ListGroupsRequest::decode(PDecoder &pd, int16_t version)
{
    m_version = version;

    if (m_version >= 4)
    {
        int err = pd.getStringArray(m_states_filter);
        if (err != 0)
            return err;

        if (m_version >= 5)
        {
            err = pd.getStringArray(m_types_filter);
            if (err != 0)
                return err;
        }
    }
    int32_t _;
    return pd.getEmptyTaggedFieldArray(_);
}

int16_t ListGroupsRequest::key() const
{
    return apiKeyListGroups;
}

int16_t ListGroupsRequest::version() const
{
    return m_version;
}

int16_t ListGroupsRequest::header_version() const
{
    return (m_version >= 3) ? 2 : 1;
}

bool ListGroupsRequest::is_valid_version() const
{
    return m_version >= 0 && m_version <= 5;
}

bool ListGroupsRequest::is_flexible()
{
    return is_flexible_version(m_version);
}

bool ListGroupsRequest::is_flexible_version(int16_t ver)
{
    return ver >= 3;
}

KafkaVersion ListGroupsRequest::required_version() const
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