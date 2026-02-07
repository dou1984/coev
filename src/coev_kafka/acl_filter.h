#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "acl_types.h"

struct AclFilter : IEncoder, VDecoder
{
    int16_t m_version = 0;
    AclResourceType m_resource_type = AclResourceTypeUnknown;
    std::string m_resource_name;
    AclResourcePatternType m_resource_pattern_type_filter = AclResourcePatternTypeLiteral;
    std::string m_principal;
    std::string m_host;
    AclOperation m_operation = AclOperationUnknown;
    AclPermissionType m_permission_type = AclPermissionTypeUnknown;

    AclFilter() = default;
    AclFilter(int16_t v) : m_version(v)
    {
    }
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
};
