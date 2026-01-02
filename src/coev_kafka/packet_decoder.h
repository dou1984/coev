#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>
#include "errors.h"
#include "metrics.h"
#include "encoder_decoder.h"
#include "dynamic_push_decoder.h"

struct PDecoder;
using taggedFieldDecoderFunc = std::function<int(PDecoder &)>;
using taggedFieldDecoders = std::unordered_map<uint64_t, taggedFieldDecoderFunc>;

struct PDecoder
{
    virtual ~PDecoder() = default;

    virtual int getInt8(int8_t &val) = 0;
    virtual int getInt16(int16_t &val) = 0;
    virtual int getInt32(int32_t &val) = 0;
    virtual int getInt64(int64_t &val) = 0;
    virtual int getVariant(int64_t &val) = 0;
    virtual int getUVariant(uint64_t &val) = 0;
    virtual int getFloat64(double &val) = 0;
    virtual int getArrayLength(int32_t &length) = 0;
    virtual int getBool(bool &val) = 0;
    virtual int getKError(KError &err) = 0;
    virtual int getDurationMs(std::chrono::milliseconds &duration) = 0;
    virtual int getEmptyTaggedFieldArray(int32_t &length) = 0;
    virtual int getTaggedFieldArray(const taggedFieldDecoders &decoders) = 0;

    virtual int getBytes(std::string &data) = 0;
    virtual int getVariantBytes(std::string &data) = 0;
    virtual int getRawBytes(int length, std::string &data) = 0;
    virtual int getString(std::string &str) = 0;
    virtual int getNullableString(std::string &str) = 0;
    virtual int getInt32Array(std::vector<int32_t> &arr) = 0;
    virtual int getInt64Array(std::vector<int64_t> &arr) = 0;
    virtual int getStringArray(std::vector<std::string> &arr) = 0;

    virtual int remaining() = 0;
    virtual int getSubset(int length, std::shared_ptr<PDecoder> &out) = 0;
    virtual int peek(int offset, int length, std::shared_ptr<PDecoder> &out) = 0;
    virtual int peekInt8(int offset, int8_t &val) = 0;

    virtual int push(std::shared_ptr<pushDecoder> in) = 0;
    virtual int pop() = 0;

    virtual std::shared_ptr<metrics::Registry> metricRegistry() = 0;
};
