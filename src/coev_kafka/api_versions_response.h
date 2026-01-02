#pragma once

#include <vector>
#include <cstdint>
#include <chrono>

#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "request.h"

struct ApiVersionsResponseKey : VEncoder, VDecoder
{
    int16_t Version = 0;
    int16_t ApiKey = 0;
    int16_t MinVersion = 0;
    int16_t MaxVersion = 0;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct ApiVersionsResponse : protocolBody
{
    int16_t Version = 0;
    int16_t ErrorCode = 0;
    std::vector<ApiVersionsResponseKey> ApiKeys;
    std::chrono::milliseconds ThrottleTime;
    ApiVersionsResponse() = default;
    ApiVersionsResponse(int16_t v) : Version(v)
    {
    }

    void setVersion(int16_t v);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;

    static PDecoder &downgradeFlexibleDecoder(PDecoder &pd);
};
