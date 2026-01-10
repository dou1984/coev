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
    Resource m_resource;
    Acl m_acl;
    AclCreation() = default;
    AclCreation(Resource _resource, Acl _acl) : m_resource(_resource), m_acl(_acl)
    {
    }
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct CreateAclsRequest : protocol_body
{

    int16_t m_version;
    std::vector<std::shared_ptr<AclCreation>> m_acl_creations;
    CreateAclsRequest() = default;
    CreateAclsRequest(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
