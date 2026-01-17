#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "packet_encoder.h"

#include "errors.h"
#include "version.h"

struct realEncoder : packetEncoder
{

    std::string m_raw;
    size_t m_offset = 0;
    std::vector<std::shared_ptr<pushEncoder>> m_stack;
    realEncoder() = default;
    realEncoder(size_t capacity)
    {
        m_raw.resize(capacity);
    }

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
    int putRawBytes(const std::string &in);
    int putBytes(const std::string &in);
    int putVariantBytes(const std::string &in);
    int putString(const std::string &in);
    int putNullableString(const std::string &in);
    int putStringArray(const std::vector<std::string> &in);
    int putInt32Array(const std::vector<int32_t> &in);
    int putNullableInt32Array(const std::vector<int32_t> &in);
    int putInt64Array(const std::vector<int64_t> &in);
    void putEmptyTaggedFieldArray();
    int offset() const { return static_cast<int>(m_offset); }
    void push(std::shared_ptr<pushEncoder> in);
    int pop();
};

struct realFlexibleEncoder : realEncoder
{
    using base = realEncoder;
    realFlexibleEncoder(const realEncoder &re)
    {
        *this = re;
    }
    realFlexibleEncoder(std::shared_ptr<realEncoder> re)
    {
        *this = *re;
    }

    int putArrayLength(int in);
    int putBytes(const std::string &in);
    int putString(const std::string &in);
    int putNullableString(const std::string &in);
    int putStringArray(const std::vector<std::string> &in);
    int putInt32Array(const std::vector<int32_t> &in);
    int putNullableInt32Array(const std::vector<int32_t> &in);
    void putEmptyTaggedFieldArray();
};
int encodeVariant(uint8_t *buf, int64_t x);