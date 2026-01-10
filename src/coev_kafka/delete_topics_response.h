#pragma once

#include <string>
#include <map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct DeleteTopicsResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, KError> m_topic_error_codes;

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
