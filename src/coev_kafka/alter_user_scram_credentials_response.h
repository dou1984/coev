/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "version.h"
#include "errors.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

namespace coev::kafka
{

    struct AlterUserScramCredentialsResult
    {
        std::string m_user;
        KError m_code = ErrNoError;
        std::string m_message;
    };

    struct AlterUserScramCredentialsResponse : protocol_body
    {
        int16_t m_version = 0;
        std::chrono::milliseconds m_throttle_time = std::chrono::milliseconds(0);
        std::vector<AlterUserScramCredentialsResult> m_results;

        AlterUserScramCredentialsResponse() : m_version(0), m_throttle_time(0) {}
        AlterUserScramCredentialsResponse(int16_t v) : m_version(v)
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
    };
}