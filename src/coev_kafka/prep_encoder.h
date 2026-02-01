#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

#include "packet_encoder.h"
#include "version.h"

struct prepEncoder : packet_encoder
{
    prepEncoder();

    void putInt8(int8_t in);
    void putInt16(int16_t in);
    void putInt32(int32_t in);
    void putInt64(int64_t in);
    void putVariant(int64_t in);
    void putUVarint(uint64_t in);
    void putFloat64(double in);
    int putArrayLength(int32_t in);
    void putBool(bool in);
    void putKError(KError in);
    void putDurationMs(std::chrono::milliseconds in);

    int putBytes(const std::string &in);
    int putVariantBytes(const std::string &in);
    int putRawBytes(const std::string &in);
    int putString(const std::string &in);
    int putNullableString(const std::string &in);
    int putStringArray(const std::vector<std::string> &in);
    int putInt32Array(const std::vector<int32_t> &in);
    int putInt64Array(const std::vector<int64_t> &in);
    int putNullableInt32Array(const std::vector<int32_t> &in);
    void putEmptyTaggedFieldArray();

    int offset() const;
    void push(push_encoder &in);
    int pop();

    std::vector<push_encoder *> m_stack;
    int m_length = 0;
};
