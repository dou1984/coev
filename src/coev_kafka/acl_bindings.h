#pragma once

#include <string>
#include <vector>
#include <memory>
#include <coev/log.h>
#include "acl_types.h"
#include "packet_encoder.h"
#include "packet_decoder.h"

struct Resource : VDecoder, VEncoder
{
    AclResourceType m_resource_type = AclResourceTypeUnknown;
    std::string m_resource_name;
    mutable AclResourcePatternType m_resource_pattern_type = AclResourcePatternTypeLiteral;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct Acl : VDecoder, IEncoder
{
    std::string m_principal;
    std::string m_host;
    AclOperation m_operation = AclOperationUnknown;
    AclPermissionType m_permission_type = AclPermissionTypeUnknown;

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct ResourceAcls : VDecoder, VEncoder
{
    Resource m_resource;
    std::vector<Acl> m_acls;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};
