#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "acl_bindings.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "acl_bindings.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct AclCreation : VDecoder, VEncoder
{
    Resource resource;
    Acl acl;
    AclCreation() = default;
    AclCreation(Resource _resource, Acl _acl) : resource(_resource), acl(_acl)
    {
    }
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct CreateAclsRequest : protocolBody
{

    int16_t Version;
    std::vector<std::shared_ptr<AclCreation>> AclCreations;
    CreateAclsRequest() = default;
    CreateAclsRequest(int16_t v) : Version(v)
    {
    }

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
};
