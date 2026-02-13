#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <memory>
#include "errors.h"
#include "version.h"
#include "encoder_decoder.h"
#include "packet_encoder.h"

struct real_encoder : packet_encoder
{

    real_encoder() = default;
    real_encoder(size_t capacity);

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

    std::string m_raw;
    size_t m_offset = 0;
    std::vector<push_encoder *> m_stack;
};
