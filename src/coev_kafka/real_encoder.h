/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include <chrono>
#include "errors.h"
#include "version.h"
#include "encoder_decoder.h"
#include "flexible_type.h"
#include "dynamic_push_encoder.h"

namespace coev::kafka
{

    struct real_encoder : FlexibleType
    {
        real_encoder() = default;
        real_encoder(size_t capacity);
        real_encoder(std::string_view buf);

        void putInt8(int8_t in);
        void putInt16(int16_t in);
        void putInt32(int32_t in);
        void putInt64(int64_t in);
        void putVariant(int64_t in);
        void putUVarint(uint64_t in);
        void putFloat64(double in);
        void putBool(bool in);
        void putKError(KError in);
        void putDurationMs(std::chrono::milliseconds ms);
        int putArrayLength(int in);
        int putRawBytes(const std::string_view &in);
        int putBytes(const std::string_view &in);
        int putVariantBytes(const std::string_view &in);
        int putString(const std::string_view &in);
        int putNullableString(const std::string_view &in);
        int putStringArray(const std::vector<std::string> &in);
        int putInt32Array(const std::vector<int32_t> &in);
        int putNullableInt32Array(const std::vector<int32_t> &in);
        int putInt64Array(const std::vector<int64_t> &in);
        void putEmptyTaggedFieldArray();
        int offset() const { return static_cast<int>(m_offset); }
        void push(push_encoder &in);
        int pop();

        std::vector<push_encoder *> m_stack;
        size_t m_offset = 0;
        std::string m_raw;
    };

} // namespace coev::kafka
