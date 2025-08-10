#include <gtest/gtest.h>

#include "compressZ.h"

using namespace coev::compress;
std::string str_default = "hello world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
TEST(test_compress, gzip)
{

    std::string compressed;
    gzipZ::Compress(compressed, str_default.c_str(), str_default.size());

    std::string decompressed;
    gzipZ::Decompress(decompressed, compressed.c_str(), compressed.size());

    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, zlib)
{
    std::string compressed;
    zlibX::Compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    zlibX::Decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, snappy)
{
    std::string compressed;
    snappyZ::Compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    snappyZ::Decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, lz4)
{
    std::string compressed;
    lz4Z::Compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    lz4Z::Decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}