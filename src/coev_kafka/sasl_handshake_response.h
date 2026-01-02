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

struct SaslHandshakeResponse : protocolBody
{
    int16_t Version = 0;
    KError Err = ErrNoError;
    std::vector<std::string> EnabledMechanisms;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t headerVersion()const;
    bool isValidVersion()const;
    KafkaVersion requiredVersion()const;
};
