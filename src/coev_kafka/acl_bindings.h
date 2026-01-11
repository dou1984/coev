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
    AclResourceType m_resource_type;
    std::string m_resource_name;
    AclResourcePatternType m_resource_pattern_type;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct Acl : VDecoder, IEncoder
{
    std::string m_principal;
    std::string m_host;
    AclOperation m_operation;
    AclPermissionType m_permission_type;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct ResourceAcls : VDecoder, VEncoder
{

    Resource m_resource;
    std::vector<std::shared_ptr<Acl>> m_acls;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};
