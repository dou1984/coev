#pragma once

#include <string>
#include <vector>
#include <memory>
#include <coev/log.h>
#include "acl_types.h"
#include "packet_encoder.h"
#include "packet_decoder.h"

struct Resource : versioned_decoder, versioned_encoder
{
    AclResourceType m_resource_type;
    std::string m_resource_name;
    mutable AclResourcePatternType m_resource_pattern_type;

    int encode(packetEncoder &pe, int16_t version) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct Acl : versioned_decoder, IEncoder
{
    std::string m_principal;
    std::string m_host;
    AclOperation m_operation;
    AclPermissionType m_permission_type;

    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct ResourceAcls : versioned_decoder, versioned_encoder
{

    Resource m_resource;
    std::vector<std::shared_ptr<Acl>> m_acls;

    int encode(packetEncoder &pe, int16_t version) const;
    int decode(packetDecoder &pd, int16_t version);
};
