#pragma once

#include <chrono>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct EndTxnResponse : protocolBody
{

    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError Err{};

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
