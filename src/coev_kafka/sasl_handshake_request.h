#pragma once

#include <string>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"

#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct SaslHandshakeRequest : protocol_body
{

    std::string m_mechanism;
    int16_t m_version = 0;
    SaslHandshakeRequest() = default;
    SaslHandshakeRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
