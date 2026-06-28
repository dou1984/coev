/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "acl_types.h"

namespace coev::kafka
{

    struct AclFilter : IEncoder, VDecoder
    {
        int16_t m_version = 0;
        AclResourceType m_resource_type = AclResourceTypeAny;
        std::string m_resource_name;
        AclResourcePatternType m_resource_pattern_type_filter = AclResourcePatternTypeLiteral;
        std::string m_principal;
        std::string m_host;
        AclOperation m_operation = AclOperationAny;
        AclPermissionType m_permission_type = AclPermissionTypeAny;

        AclFilter() = default;
        AclFilter(int16_t v) : m_version(v)
        {
        }
        int encode(PacketEncoder &pe) const;
        int decode(PacketDecoder &pd, int16_t version);
    };

} // namespace coev::kafka
