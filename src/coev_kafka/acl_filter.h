#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "acl_types.h"

struct AclFilter : IEncoder, VDecoder
{
    int16_t Version;
    AclResourceType ResourceType;
    std::string ResourceName;
    AclResourcePatternType ResourcePatternTypeFilter;
    std::string Principal;
    std::string Host;
    AclOperation Operation;
    AclPermissionType PermissionType;

    AclFilter() = default;
    AclFilter(int16_t v) : Version(v)
    {
    }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};
