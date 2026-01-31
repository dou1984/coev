#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "errors.h"
#include "protocol_body.h"

struct Broker;
struct FindCoordinatorResponse : protocol_body, throttle_support
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_err = ErrNoError;
    std::string m_err_msg;
    std::shared_ptr<Broker> m_coordinator;

    void set_version(int16_t v);
    int decode(packetDecoder &pd, int16_t version);
    int encode(packetEncoder &pe) const;
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
