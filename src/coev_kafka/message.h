#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>
#include "config.h"
#include "timestamp.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "compress.h"

struct MessageSet;

std::string toString(CompressionCodec codec);
bool fromString(const std::string &s, CompressionCodec &out);

struct Message
{
    CompressionCodec m_codec = CompressionCodec::None;
    int m_compression_level = CompressionLevelDefault;
    bool m_log_append_time = false;
    std::string m_key;
    std::string m_value;
    std::shared_ptr<MessageSet> m_set;
    int8_t m_version = 0;
    Timestamp m_timestamp;

    std::string m_compressed_cache;
    int m_compressed_size = 0;

    Message() = default;

    Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
    int decodeSet();
};

std::string compress(CompressionCodec codec, int level, const std::string &data);
std::string decompress(CompressionCodec codec, const std::string &data);
