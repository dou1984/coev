#include <gtest/gtest.h>

#include "../compress/compress.h"

std::string str_default = "hello world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
TEST(test_compress, gzip)
{

    coev::compress::gzipX x;

    std::string compressed;
    x.compress(compressed, str_default.c_str(), str_default.size());

    std::string decompressed;
    x.decompress(decompressed, compressed.c_str(), compressed.size());

    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, zlib)
{
    coev::compress::zlibX x;
    std::string compressed;
    x.compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    x.decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, snappy)
{
    coev::compress::snappyX x;
    std::string compressed;
    x.compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    x.decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, lz4)
{
    coev::compress::lz4X x;
    std::string compressed;
    x.compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    x.decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}