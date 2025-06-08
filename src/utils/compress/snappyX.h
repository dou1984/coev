#pragma once
#include <string>
#include <cstdint>

namespace coev::compress
{
    class snappyX
    {

    public:
        int compress(std::string &compressed, const char *buf, size_t buf_size);
        int decompress(std::string &decompressd, const char *buf, size_t buf_size);
    };
}