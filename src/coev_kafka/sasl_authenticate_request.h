#pragma once

#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct SaslAuthenticateRequest : protocol_body
{

    int16_t m_version;
    std::string m_sasl_auth_bytes;

    SaslAuthenticateRequest() = default;
    SaslAuthenticateRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
