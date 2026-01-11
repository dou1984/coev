#pragma once
#include <cstdint>
#include <vector>
#include <string>

enum CompressionCodec : int8_t
{
    None = 0,
    GZIP = 1,
    Snappy = 2,
    LZ4 = 3,
    ZSTD = 4
};

inline constexpr int8_t CompressionCodecMask = 0x07;
inline constexpr int8_t TimestampTypeMask = 0x08;
inline constexpr int CompressionLevelDefault = -1000;

int Compress(CompressionCodec cc, int level, const std::string &data, std::string &out);
int Decompress(CompressionCodec cc, const std::string &data, std::string &out);
