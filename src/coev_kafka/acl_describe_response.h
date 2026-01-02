#pragma once

#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <cstdint>
#include "acl_bindings.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "protocol_body.h"

struct DescribeAclsResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::vector<std::shared_ptr<ResourceAcls>> ResourceAcls_;
    std::string ErrMsg;
    KError Err;

    DescribeAclsResponse() = default;
    DescribeAclsResponse(int16_t v) : Version(v)
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
    std::chrono::milliseconds throttleTime() const;
};
