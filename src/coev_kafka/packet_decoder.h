/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string_view>
#include <coev/coev.h>
#include "errors.h"
#include "flexible_type.h"
#include "encoder_decoder.h"
#include "dynamic_push_decoder.h"

namespace coev::kafka
{
    struct packet_decoder;
    using taggedFieldDecoderFunc = std::function<int(packet_decoder &)>;
    using taggedFieldDecoders = std::unordered_map<uint64_t, taggedFieldDecoderFunc>;

    struct packet_decoder : flexible_type
    {
        int getInt8(int8_t &result);
        int getInt16(int16_t &result);
        int getInt32(int32_t &result);
        int getInt64(int64_t &result);
        int getVariant(int64_t &result);
        int getUVariant(uint64_t &result);
        int getFloat64(double &result);
        int getArrayLength(int32_t &result);
        int getBool(bool &result);
        int getKError(KError &result);
        int getDurationMs(std::chrono::milliseconds &duration);
        int getTaggedFieldArray(const taggedFieldDecoders &decoders);
        int getEmptyTaggedFieldArray(int &result);
        int getStringLength(int &result);
        int getBytes(std::string_view &result);
        int getBytes(std::string &result);
        int getVariantBytes(std::string_view &result);
        int getVariantBytes(std::string &result);
        int getString(std::string_view &result);
        int getString(std::string &result);
        int getNullableString(std::string_view &result);
        int getNullableString(std::string &result);
        int getInt32Array(std::vector<int32_t> &result);
        int getInt64Array(std::vector<int64_t> &result);
        int getStringArray(std::vector<std::string> &result);
        int remaining();
        int getRawBytes(int length, std::string_view &result);
        int getSubset(int length, std::string_view &result);
        int peek(int offset, int length, std::string_view &result);
        int peekInt8(int offset, int8_t &result);
        int push(push_decoder &in);
        int pop();

        packet_decoder() = default;
        packet_decoder(const std::string &buf);
        packet_decoder(std::string_view buf);

        int m_offset = 0;
        std::string_view m_raw;
        std::vector<push_decoder *> m_stack;
    };
}