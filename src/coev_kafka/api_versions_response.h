/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <vector>
#include <cstdint>
#include <chrono>

#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "request.h"

namespace coev::kafka
{

    struct ApiVersionsResponseKey : VEncoder, VDecoder
    {
        int16_t m_version = 0;
        int16_t m_api_key = 0;
        int16_t m_min_version = 0;
        int16_t m_max_version = 0;

        int encode(PacketEncoder &pe, int16_t version) const;
        int decode(PacketDecoder &pd, int16_t version);
    };

    struct ApiVersionsResponse : protocol_body
    {
        int16_t m_version = 0;
        int16_t m_code = 0;
        std::vector<ApiVersionsResponseKey> m_api_keys;
        std::chrono::milliseconds m_throttle_time;
        ApiVersionsResponse() = default;
        ApiVersionsResponse(int16_t v) : m_version(v)
        {
        }

        void set_version(int16_t v);

        int encode(PacketEncoder &pe) const;
        int decode(PacketDecoder &pd, int16_t version);

        int16_t key() const;
        int16_t version() const;
        int16_t header_version() const;
        bool is_valid_version() const;
        bool is_flexible() const;
        bool is_flexible_version(int16_t version) const;
        KafkaVersion required_version() const;
        std::chrono::milliseconds throttle_time() const;

        static PacketDecoder &downgrade_flexible_decoder(PacketDecoder &pd);
    };

} // namespace coev::kafka
