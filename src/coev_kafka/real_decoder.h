#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string_view>
#include <coev/coev.h>
#include "packet_decoder.h"
#include "errors.h"
#include "encoder_decoder.h"

struct real_decoder : packet_decoder
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
    int getBytes(std::string &result);
    int getVariantBytes(std::string &result);
    int getStringLength(int &result);
    int getString(std::string &result);
    int getNullableString(std::string &result);
    int getInt32Array(std::vector<int32_t> &result);
    int getInt64Array(std::vector<int64_t> &result);
    int getStringArray(std::vector<std::string> &result);
    int remaining();
    int getSubset(int length, std::shared_ptr<packet_decoder> &result);
    int getRawBytes(int length, std::string &result);
    int peek(int offset, int length, std::shared_ptr<packet_decoder> &result);
    int peekInt8(int offset, int8_t &result);
    int push(push_decoder &in);
    int pop();

    int m_offset = 0;
    std::string_view m_raw;
    std::vector<push_decoder *> m_stack;
};
