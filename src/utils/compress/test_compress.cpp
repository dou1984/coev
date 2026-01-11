#include <gtest/gtest.h>

#include "coev_compress.h"

std::string str_default = "hello world!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
TEST(test_compress, gzip)
{

    std::string compressed;
    coev::gzip::Compress(compressed, str_default.c_str(), str_default.size());

    std::string decompressed;
    coev::gzip::Decompress(decompressed, compressed.c_str(), compressed.size());

    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, zlib)
{
    std::string compressed;
    coev::zlib::Compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    coev::zlib::Decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, snappy)
{
    std::string compressed;
    coev::snappy::Compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    coev::snappy::Decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}

TEST(test_compress, lz4)
{
    std::string compressed;
    coev::lz4::Compress(compressed, str_default.c_str(), str_default.size());
    std::string decompressed;
    coev::lz4::Decompress(decompressed, compressed.c_str(), compressed.size());
    ASSERT_EQ(str_default, decompressed);
}