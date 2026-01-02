#pragma once

#include <string>
#include <vector>
#include <memory>

#include "acl_types.h"
#include "packet_encoder.h"
#include "packet_decoder.h"

struct Resource : VDecoder, VEncoder
{
    AclResourceType ResourceType;
    std::string ResourceName;
    AclResourcePatternType ResourcePatternType;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct Acl : VDecoder, IEncoder
{
    std::string Principal;
    std::string Host;
    AclOperation Operation;
    AclPermissionType PermissionType;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct ResourceAcls : VDecoder, VEncoder
{

    Resource Resource_;
    std::vector<std::shared_ptr<Acl>> Acls;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};
