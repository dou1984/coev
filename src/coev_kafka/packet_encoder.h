/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <chrono>
#include "errors.h"
#include "encoder_decoder.h"
#include "dynamic_push_encoder.h"
#include "prep_encoder.h"
#include "real_encoder.h"

namespace coev::kafka
{
    struct packet_encoder : flexible_type
    {
        enum Type : uint8_t
        {
            NONE,
            PREP,
            REAL,
        };

        int m_type = NONE;
        union
        {
            size_t m_offset = 0;
            size_t m_length;
        };
        std::vector<push_encoder *> m_stack;
        std::string m_raw;

        packet_encoder(Type type);
        packet_encoder(Type type, size_t l);
        packet_encoder(std::string_view buf);
        virtual ~packet_encoder();

        void putInt8(int8_t in);
        void putInt16(int16_t in);
        void putInt32(int32_t in);
        void putInt64(int64_t in);
        void putVariant(int64_t in);
        void putUVarint(uint64_t in);
        void putFloat64(double in);
        void putBool(bool in);
        void putKError(KError in);
        void putDurationMs(std::chrono::milliseconds in);
        int putArrayLength(int32_t in);
        int putBytes(const std::string_view &in);
        int putVariantBytes(const std::string_view &in);
        int putRawBytes(const std::string_view &in);
        int putString(const std::string_view &in);
        int putNullableString(const std::string_view &in);
        int putStringArray(const std::vector<std::string> &in);
        int putInt32Array(const std::vector<int32_t> &in);
        int putInt64Array(const std::vector<int64_t> &in);
        int putNullableInt32Array(const std::vector<int32_t> &in);
        void putEmptyTaggedFieldArray();
        int offset() const;
        void push(push_encoder &in);
        int pop();
    };

    int encodeUVariant(uint8_t *buf, uint64_t x);
    int encodeVariant(uint8_t *buf, int64_t x);
}