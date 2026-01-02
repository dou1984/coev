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
struct FindCoordinatorResponse : protocolBody, throttleSupport
{

    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError Err = ErrNoError;
    std::string ErrMsg;
    std::shared_ptr<Broker> Coordinator;

    void setVersion(int16_t v);
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
