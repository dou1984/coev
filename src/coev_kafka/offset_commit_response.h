// offset_commit_response.h
#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"

#include "api_versions.h"
#include "protocol_body.h"

struct OffsetCommitResponse :  protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::unordered_map<std::string, std::unordered_map<int32_t, KError>> m_errors;

    OffsetCommitResponse();

    void set_version(int16_t v);
    void AddError(const std::string &topic, int32_t partition, KError kerror);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
