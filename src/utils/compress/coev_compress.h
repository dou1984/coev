#pragma once
#include "coev_gzip.h"
#include "coev_zlib.h"
#include "coev_snappy.h"
#include "coev_lz4.h"
#include "coev_zstd.h"

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
