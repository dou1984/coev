#pragma once

#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "errors.h"
#include "protocol_body.h"

struct HeartbeatResponse : protocol_body
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_err = ErrNoError;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version()const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion required_version()const;
    std::chrono::milliseconds throttle_time() const;
};
