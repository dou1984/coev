/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <cstdint>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "version.h"
#include "throttle_support.h"

namespace coev::kafka
{

    struct protocol_body : VDecoder, HEncoder
    {
        virtual ~protocol_body() = default;

        virtual int16_t key() const = 0;
        virtual int16_t version() const = 0;
        virtual void set_version(int16_t version) = 0;
        virtual bool is_valid_version() const = 0;
        virtual KafkaVersion required_version() const = 0;
    };

} // namespace coev::kafka
