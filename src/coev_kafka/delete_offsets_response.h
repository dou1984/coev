#pragma once

#include <string>
#include <map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "protocol_body.h"

struct DeleteOffsetsResponse : protocolBody
{
    int16_t Version;
    KError ErrorCode;
    std::chrono::milliseconds ThrottleTime; // in milliseconds
    std::unordered_map<std::string, std::map<int32_t, KError>> Errors;
    DeleteOffsetsResponse() = default;
    DeleteOffsetsResponse(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    void AddError(const std::string &topic, int32_t partition, KError errorCode);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
