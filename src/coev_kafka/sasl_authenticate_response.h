#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "errors.h"
#include "version.h"
#include "protocol_body.h"

struct SaslAuthenticateResponse : protocol_body
{
    int16_t m_version = 0;
    KError m_err = ErrNoError;
    std::string m_error_message;
    std::string m_sasl_auth_bytes;
    int64_t m_session_lifetime_ms = 0;

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version()const;
    KafkaVersion required_version()const;
};

using AuthSendReceiver = std::function<coev::awaitable<int>(const std::string &, std::shared_ptr<SaslAuthenticateResponse> &)>;