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

struct SaslAuthenticateResponse : protocolBody
{
    int16_t Version = 0;
    KError Err = ErrNoError;
    std::string ErrorMessage;
    std::string SaslAuthBytes;
    int64_t SessionLifetimeMs = 0;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t headerVersion()const;
    bool isValidVersion()const;
    KafkaVersion requiredVersion()const;
};

using AuthSendReceiver = std::function<coev::awaitable<int>(const std::string &, std::shared_ptr<SaslAuthenticateResponse> &)>;