#include "version.h"
#include "acl_filter.h"

int AclFilter::encode(PEncoder &pe)
{
    pe.putInt8(static_cast<int8_t>(ResourceType));

    if (pe.putNullableString(ResourceName) != 0)
    {
        return -1;
    }

    if (Version == 1)
    {
        pe.putInt8(static_cast<int8_t>(ResourcePatternTypeFilter));
    }

    if (pe.putNullableString(Principal) != 0)
    {
        return -1;
    }

    if (pe.putNullableString(Host) != 0)
    {
        return -1;
    }

    pe.putInt8(static_cast<int8_t>(Operation));
    pe.putInt8(static_cast<int8_t>(PermissionType));

    return 0;
}

int AclFilter::decode(PDecoder &pd, int16_t version)
{
    int8_t resourceType;
    if (pd.getInt8(resourceType) != 0)
    {
        return -1;
    }
    ResourceType = static_cast<AclResourceType>(resourceType);

    if (pd.getNullableString(ResourceName) != 0)
    {
        return -1;
    }

    if (Version == 1)
    {
        int8_t pattern;
        if (pd.getInt8(pattern) != 0)
        {
            return -1;
        }
        ResourcePatternTypeFilter = static_cast<AclResourcePatternType>(pattern);
    }

    if (pd.getNullableString(Principal) != 0)
    {
        return -1;
    }

    if (pd.getNullableString(Host) != 0)
    {
        return -1;
    }

    int8_t operation;
    if (pd.getInt8(operation) != 0)
    {
        return -1;
    }
    Operation = static_cast<AclOperation>(operation);

    int8_t permissionType;
    if (pd.getInt8(permissionType) != 0)
    {
        return -1;
    }
    PermissionType = static_cast<AclPermissionType>(permissionType);

    return 0;
}