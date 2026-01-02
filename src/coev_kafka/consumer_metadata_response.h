#pragma once

#include <memory>
#include <string>

#include "packet_decoder.h"
#include "packet_encoder.h"
#include "errors.h"
#include "version.h"
#include "protocol_body.h"

struct Broker;
struct ConsumerMetadataResponse : protocolBody
{

    int16_t Version;
    KError Err;
    std::shared_ptr<Broker> Coordinator;
    int32_t CoordinatorID;
    std::string CoordinatorHost;
    int32_t CoordinatorPort;
    ConsumerMetadataResponse() = default;
    ConsumerMetadataResponse(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe);
};
