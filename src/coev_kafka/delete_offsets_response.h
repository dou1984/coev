#pragma once

#include <string>
#include <map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "protocol_body.h"

struct DeleteOffsetsResponse : protocol_body
{
    int16_t m_version;
    KError m_error_code;
    std::chrono::milliseconds m_throttle_time;
    std::unordered_map<std::string, std::map<int32_t, KError>> m_errors;
    DeleteOffsetsResponse() = default;
    DeleteOffsetsResponse(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    void AddError(const std::string &topic, int32_t partition, KError errorCode);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
