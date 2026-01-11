#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "errors.h"
#include "version.h"
#include "protocol_body.h"

struct SaslHandshakeResponse : protocol_body
{
    int16_t m_version = 0;
    KError m_err = ErrNoError;
    std::vector<std::string> m_enabled_mechanisms;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version()const;
    KafkaVersion required_version()const;
};
