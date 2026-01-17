#pragma once

#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "errors.h"
#include "protocol_body.h"

struct InitProducerIDResponse : protocol_body , flexible_version
{
    std::chrono::milliseconds m_throttle_time;
    KError m_err = ErrNoError;
    int16_t m_version = 0;
    int64_t m_producer_id = 0;
    int16_t m_producer_epoch = 0;

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version()const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t ver) const;
    KafkaVersion required_version()const;
    std::chrono::milliseconds throttle_time() const;
};
