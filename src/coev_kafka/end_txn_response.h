#pragma once

#include <chrono>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct EndTxnResponse : protocol_body
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_err;

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
