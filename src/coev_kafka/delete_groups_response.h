#pragma once

#include <string>
#include <map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"
struct DeleteGroupsResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, KError> m_group_error_codes;
    DeleteGroupsResponse() = default;
    DeleteGroupsResponse(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;
};
