#pragma once

#include <chrono>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "throttle_support.h"

struct AddOffsetsToTxnResponse : protocolBody, throttleSupport
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    KError Err;

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
