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
    int16_t m_version = 0;
    int16_t m_api_key = 0;
    int16_t m_min_version = 0;
    int16_t m_max_version = 0;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct ApiVersionsResponse : protocol_body
{
    int16_t m_version = 0;
    int16_t m_error_code = 0;
    std::vector<ApiVersionsResponseKey> m_api_keys;
    std::chrono::milliseconds m_throttle_time;
    ApiVersionsResponse() = default;
    ApiVersionsResponse(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;

    static PDecoder &downgradeFlexibleDecoder(PDecoder &pd);
};
