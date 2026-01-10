#pragma once

#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "errors.h"
#include "protocol_body.h"

struct InitProducerIDResponse : protocol_body
{
    std::chrono::milliseconds m_throttle_time;
    KError m_err = ErrNoError;
    int16_t m_version = 0;
    int64_t m_producer_id = 0;
    int16_t m_producer_epoch = 0;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t headerVersion()const;
    bool is_valid_version()const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion required_version()const;
    std::chrono::milliseconds throttleTime() const;
};
