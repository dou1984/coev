#pragma once

#include <vector>
#include <memory>
#include <cstdint>

#include "acl_filter.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct DeleteAclsRequest : protocolBody
{

    int16_t Version;
    std::vector<std::shared_ptr<AclFilter>> Filters;
    DeleteAclsRequest() = default;
    DeleteAclsRequest(int16_t v) : Version(v)
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
