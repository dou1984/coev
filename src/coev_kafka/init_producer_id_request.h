#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct InitProducerIDRequest : protocolBody
{

    int16_t Version = 0;
    std::string TransactionalID;
    std::chrono::milliseconds TransactionTimeout;
    int64_t ProducerID = 0;
    int16_t ProducerEpoch = 0;
    InitProducerIDRequest() = default;
    InitProducerIDRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;
};
