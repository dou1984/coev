#pragma once
#include <cstdint>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "protocol_body.h"
#include "version.h"
#include "throttle_support.h"

struct protocolBody : VDecoder, encoderWithHeader
{
    virtual ~protocolBody() = default;

    virtual int16_t key() const = 0;
    virtual int16_t version() const = 0;
    virtual void setVersion(int16_t version) = 0;
    virtual bool isValidVersion() const = 0;
    virtual KafkaVersion requiredVersion() const = 0;
};
