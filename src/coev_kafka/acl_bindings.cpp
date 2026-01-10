#include "acl_bindings.h"
#include <cstdint>
#include <iostream>
#include "logger.h"

int Resource::encode(PEncoder &pe, int16_t version)
{
    pe.putInt8(static_cast<int8_t>(m_resource_type));

    if (pe.putString(m_resource_name) != 0)
    {
        return -1;
    }

    if (version == 1)
    {
        if (m_resource_pattern_type == static_cast<AclResourcePatternType>(-1))
        { // AclPatternUnknown
            Logger::Println("Cannot encode an unknown resource pattern type, using Literal instead");
            m_resource_pattern_type = static_cast<AclResourcePatternType>(0); // AclPatternLiteral
        }
        pe.putInt8(static_cast<int8_t>(m_resource_pattern_type));
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
    m_resource_type = static_cast<AclResourceType>(resourceType);

    if (pd.getString(m_resource_name) != 0)
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
        m_resource_pattern_type = static_cast<AclResourcePatternType>(pattern);
    }

    return 0;
}

int Acl::encode(PEncoder &pe)
{
    if (pe.putString(m_principal) != 0)
    {
        return -1;
    }

    if (pe.putString(m_host) != 0)
    {
        return -1;
    }

    pe.putInt8(static_cast<int8_t>(m_operation));
    pe.putInt8(static_cast<int8_t>(m_permission_type));

    return 0;
}

int Acl::decode(PDecoder &pd, int16_t version)
{
    if (pd.getString(m_principal) != 0)
    {
        return -1;
    }

    if (pd.getString(m_host) != 0)
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

int ResourceAcls::encode(PEncoder &pe, int16_t version)
{
    if (m_resource.encode(pe, version) != 0)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_acls.size())) != 0)
    {
        return -1;
    }

    for (auto &acl : m_acls)
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
    if (m_resource.decode(pd, version) != 0)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != 0)
    {
        return -1;
    }

    m_acls.resize(n);
    for (int i = 0; i < n; ++i)
    {
        m_acls[i] = std::make_shared<Acl>();
        if (m_acls[i]->decode(pd, version) != 0)
        {
            return -1;
        }
    }

    return 0;
}