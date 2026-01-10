#include "version.h"
#include "acl_filter.h"

int AclFilter::encode(PEncoder &pe)
{
    pe.putInt8(static_cast<int8_t>(m_resource_type));

    if (pe.putNullableString(m_resource_name) != 0)
    {
        return -1;
    }

    if (m_version == 1)
    {
        pe.putInt8(static_cast<int8_t>(m_resource_pattern_type_filter));
    }

    if (pe.putNullableString(m_principal) != 0)
    {
        return -1;
    }

    if (pe.putNullableString(m_host) != 0)
    {
        return -1;
    }

    pe.putInt8(static_cast<int8_t>(m_operation));
    pe.putInt8(static_cast<int8_t>(m_permission_type));

    return 0;
}

int AclFilter::decode(PDecoder &pd, int16_t version)
{
    int8_t resourceType;
    if (pd.getInt8(resourceType) != 0)
    {
        return -1;
    }
    m_resource_type = static_cast<AclResourceType>(resourceType);

    if (pd.getNullableString(m_resource_name) != 0)
    {
        return -1;
    }

    if (m_version == 1)
    {
        int8_t pattern;
        if (pd.getInt8(pattern) != 0)
        {
            return -1;
        }
        m_resource_pattern_type_filter = static_cast<AclResourcePatternType>(pattern);
    }

    if (pd.getNullableString(m_principal) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(m_host) != 0)
    {
        return -1;
    }

    int8_t operation;
    if (pd.getInt8(operation) != 0)
    {
        return -1;
    }
    m_operation = static_cast<AclOperation>(operation);

    int8_t permissionType;
    if (pd.getInt8(permissionType) != 0)
    {
        return -1;
    }
    m_permission_type = static_cast<AclPermissionType>(permissionType);

    return 0;
}