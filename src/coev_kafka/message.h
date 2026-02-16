#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>
#include <memory>
#include <string_view>
#include <utils/compress/coev_compress.h>
#include "config.h"
#include "timestamp.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "message_set.h"

std::string ToString(CompressionCodec codec);
bool FromString(const std::string &s, CompressionCodec &out);

struct Message
{
    CompressionCodec m_codec = CompressionCodec::None;
    int m_compression_level = CompressionLevelDefault;
    bool m_log_append_time = false;
    std::string m_key;
    std::string m_value;
    MessageSet m_message_set;
    int8_t m_version = 0;
    Timestamp m_timestamp;

    mutable std::string m_compressed_cache;
    mutable int m_compressed_size = 0;

    Message() = default;

    Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
    int decode_set();
    void clear();
};

int compress(CompressionCodec codec, int level, std::string_view data, std::string &out);
int decompress(CompressionCodec codec, std::string_view data, std::string &out);
