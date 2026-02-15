#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <unordered_map>
#include "errors.h"
#include "encoder_decoder.h"
#include "dynamic_push_decoder.h"

struct packet_decoder;
using taggedFieldDecoderFunc = std::function<int(packet_decoder &)>;
using taggedFieldDecoders = std::unordered_map<uint64_t, taggedFieldDecoderFunc>;

struct packet_decoder : packet_type
{
    virtual ~packet_decoder() = default;

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
    virtual int getBytes(std::string_view &data) = 0;
    virtual int getBytes(std::string &data) = 0;
    virtual int getVariantBytes(std::string_view &data) = 0;
    virtual int getVariantBytes(std::string &data) = 0;
    virtual int getRawBytes(int length, std::string_view &data) = 0;
    virtual int getString(std::string_view &str) = 0;
    virtual int getString(std::string &str) = 0;
    virtual int getNullableString(std::string_view &str) = 0;
    virtual int getNullableString(std::string &str) = 0;
    virtual int getInt32Array(std::vector<int32_t> &arr) = 0;
    virtual int getInt64Array(std::vector<int64_t> &arr) = 0;
    virtual int getStringArray(std::vector<std::string> &arr) = 0;
    virtual int remaining() = 0;
    virtual int getSubset(int length, std::string_view &out) = 0;
    virtual int peek(int offset, int length, std::string_view &out) = 0;
    virtual int peekInt8(int offset, int8_t &val) = 0;
    virtual int push(push_decoder &in) = 0;

    virtual int pop() = 0;
};
