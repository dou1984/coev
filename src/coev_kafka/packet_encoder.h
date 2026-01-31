#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "errors.h"
#include "encoder_decoder.h"
#include "dynamic_push_encoder.h"

struct packetEncoder : packet_type
{
    virtual ~packetEncoder() = default;

    virtual void putInt8(int8_t in) = 0;
    virtual void putInt16(int16_t in) = 0;
    virtual void putInt32(int32_t in) = 0;
    virtual void putInt64(int64_t in) = 0;
    virtual void putVariant(int64_t in) = 0;
    virtual void putUVarint(uint64_t in) = 0;
    virtual void putFloat64(double in) = 0;
    virtual void putBool(bool in) = 0;
    virtual void putKError(KError in) = 0;
    virtual void putDurationMs(std::chrono::milliseconds in) = 0;

    virtual int putArrayLength(int32_t in) = 0;
    virtual int putBytes(const std::string &in) = 0;
    virtual int putVariantBytes(const std::string &in) = 0;
    virtual int putRawBytes(const std::string &in) = 0;
    virtual int putString(const std::string &in) = 0;
    virtual int putNullableString(const std::string &in) = 0;
    virtual int putStringArray(const std::vector<std::string> &in) = 0;
    virtual int putInt32Array(const std::vector<int32_t> &in) = 0;
    virtual int putInt64Array(const std::vector<int64_t> &in) = 0;
    virtual int putNullableInt32Array(const std::vector<int32_t> &in) = 0;
    virtual void putEmptyTaggedFieldArray() = 0;

    virtual int offset() const = 0;
    virtual void push(pushEncoder &in) = 0;

    virtual int pop() = 0;
};
