#include <cstdint>
#include <iostream>
#include "acl_bindings.h"

int Resource::encode(packetEncoder &pe, int16_t version)
{
    pe.putInt8(static_cast<int8_t>(m_resource_type));

    if (pe.putString(m_resource_name) != ErrNoError)
    {
        return -1;
    }

    if (version == 1)
    {
        if (m_resource_pattern_type == static_cast<AclResourcePatternType>(-1))
        {
            LOG_CORE("Resource::encode Cannot encode an unknown resource pattern type, using Literal instead");
            m_resource_pattern_type = AclResourcePatternTypeUnknown;
        }
        pe.putInt8(static_cast<int8_t>(m_resource_pattern_type));
    }

    return ErrNoError;
}

int Resource::decode(packetDecoder &pd, int16_t version)
{
    int8_t resourceType;
    if (pd.getInt8(resourceType) != ErrNoError)
    {
        return -1;
    }
    m_resource_type = static_cast<AclResourceType>(resourceType);

    if (pd.getString(m_resource_name) != ErrNoError)
    {
        return -1;
    }

    if (version == 1)
    {
        int8_t pattern;
        if (pd.getInt8(pattern) != ErrNoError)
        {
            return -1;
        }
        m_resource_pattern_type = static_cast<AclResourcePatternType>(pattern);
    }

    return ErrNoError;
}

int Acl::encode(packetEncoder &pe)
{
    if (pe.putString(m_principal) != ErrNoError)
    {
        return -1;
    }

    if (pe.putString(m_host) != ErrNoError)
    {
        return -1;
    }

    pe.putInt8(static_cast<int8_t>(m_operation));
    pe.putInt8(static_cast<int8_t>(m_permission_type));

    return ErrNoError;
}

int Acl::decode(packetDecoder &pd, int16_t version)
{
    if (pd.getString(m_principal) != ErrNoError)
    {
        return -1;
    }

    if (pd.getString(m_host) != ErrNoError)
    {
        return -1;
    }

    int8_t operation;
    if (pd.getInt8(operation) != ErrNoError)
    {
        return -1;
    }
    m_operation = static_cast<AclOperation>(operation);

    int8_t permissionType;
    if (pd.getInt8(permissionType) != ErrNoError)
    {
        return -1;
    }
    m_permission_type = static_cast<AclPermissionType>(permissionType);

    return ErrNoError;
}

int ResourceAcls::encode(packetEncoder &pe, int16_t version)
{
    if (m_resource.encode(pe, version) != ErrNoError)
    {
        return -1;
    }

    if (pe.putArrayLength(static_cast<int32_t>(m_acls.size())) != ErrNoError)
    {
        return -1;
    }

    for (auto &acl : m_acls)
    {
        if (acl->encode(pe) != ErrNoError)
        {
            return -1;
        }
    }

    return ErrNoError;
}

int ResourceAcls::decode(packetDecoder &pd, int16_t version)
{
    if (m_resource.decode(pd, version) != ErrNoError)
    {
        return -1;
    }

    int32_t n;
    if (pd.getArrayLength(n) != ErrNoError)
    {
        return -1;
    }

    m_acls.resize(n);
    for (int i = 0; i < n; ++i)
    {
        m_acls[i] = std::make_shared<Acl>();
        if (m_acls[i]->decode(pd, version) != ErrNoError)
        {
            return -1;
        }
    }

    return ErrNoError;
}