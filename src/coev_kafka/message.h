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
    CompressionCodec Codec = CompressionCodec::None;
    int CompressionLevel = CompressionLevelDefault;
    bool LogAppendTime = false;
    std::string Key;
    std::string Value;
    std::shared_ptr<MessageSet> Set;
    int8_t Version = 0;
    Timestamp Timestamp_;

    std::string compressedCache;
    int compressedSize = 0;

    Message() = default;

    Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
    int decodeSet();
};

std::string compress(CompressionCodec codec, int level, const std::string &data);
std::string decompress(CompressionCodec codec, const std::string &data);
