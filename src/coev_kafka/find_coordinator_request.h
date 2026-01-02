#pragma once

#include <string>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"

#include "api_versions.h"
#include "coordinator_type.h"
#include "protocol_body.h"

struct FindCoordinatorRequest : protocolBody
{

    int16_t Version;
    std::string CoordinatorKey;
    CoordinatorType CoordinatorType_;

    FindCoordinatorRequest();
    FindCoordinatorRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
};
