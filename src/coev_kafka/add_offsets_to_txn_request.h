#pragma once

#include <string>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "protocol_body.h"

struct AddOffsetsToTxnRequest : protocolBody
{
    int16_t Version;
    std::string TransactionalID;
    int64_t ProducerID;
    int16_t ProducerEpoch;
    std::string GroupID;
    AddOffsetsToTxnRequest() = default;
    AddOffsetsToTxnRequest(int16_t v) : Version(v)
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
