#pragma once

#include <chrono>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "throttle_support.h"

struct AddOffsetsToTxnResponse : protocol_body, throttle_support
{
    int16_t m_version;
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
