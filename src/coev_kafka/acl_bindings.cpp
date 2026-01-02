#include "acl_bindings.h"
#include <cstdint>
#include <iostream>
#include "logger.h"

int Resource::encode(PEncoder &pe, int16_t version)
{
    pe.putInt8(static_cast<int8_t>(ResourceType));

    if (pe.putString(ResourceName) != 0)
    {
        return -1;
    }

    if (version == 1)
    {
        if (ResourcePatternType == static_cast<AclResourcePatternType>(-1))
        { // AclPatternUnknown
            Logger::Println("Cannot encode an unknown resource pattern type, using Literal instead");
            ResourcePatternType = static_cast<AclResourcePatternType>(0); // AclPatternLiteral
        }
        pe.putInt8(static_cast<int8_t>(ResourcePatternType));
    }

    return 0;
}

int Resource::decode(PDecoder &pd, int16_t version)
{
    int8_t resourceType;
    if (pd.getInt8(resourceType) != 0)
    {
        return -1;
    }
    ResourceType = static_cast<AclResourceType>(resourceType);

    if (pd.getString(ResourceName) != 0)
    {
        return -1;
    }

    if (version == 1)
    {
        int8_t pattern;
        if (pd.getInt8(pattern) != 0)
        {
            return -1;
        }
        ResourcePatternType = static_cast<AclResourcePatternType>(pattern);
    }

    return 0;
}

int Acl::encode(PEncoder &pe)
{
    if (pe.putString(Principal) != 0)
    {
        return -1;
    }

    if (pe.putString(Host) != 0)
    {
        return -1;
    }

    pe.putInt8(static_cast<int8_t>(Operation));
    pe.putInt8(static_cast<int8_t>(PermissionType));

    return 0;
}

int Acl::decode(PDecoder &pd, int16_t version)
{
    if (pd.getString(Principal) != 0)
    {
        return -1;
    }

    if (pd.getString(Host) != 0)
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

int ResourceAcls::encode(PEncoder &pe, int16_t version)
{
    if (Resource_.encode(pe, version) != 0)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(Acls.size())) != 0)
    {
        return -1;
    }

    for (auto &acl : Acls)
    {
        if (acl->encode(pe) != 0)
        {
            return -1;
        }
    }

    return 0;
}

int ResourceAcls::decode(PDecoder &pd, int16_t version)
{
    if (Resource_.decode(pd, version) != 0)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    Acls.resize(n);
    for (int i = 0; i < n; ++i)
    {
        Acls[i] = std::make_shared<Acl>();
        if (Acls[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}