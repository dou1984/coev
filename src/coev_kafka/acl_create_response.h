#pragma once

#include <vector>
#include <memory>
#include <string>
#include <chrono>
#include <cstdint>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "protocol_body.h"

struct AclCreationResponse : IEncoder, VDecoder
{
    KError m_err;
    std::string m_err_msg;

    AclCreationResponse() = default;
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct CreateAclsResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<std::shared_ptr<AclCreationResponse>> m_acl_creation_responses;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;
};
