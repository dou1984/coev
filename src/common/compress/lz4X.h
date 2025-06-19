#pragma once
#include <string>
#include <cstddef>
#include <lz4.h>
#include <coev/coev.h>

namespace coev::compress
{
    class lz4X
    {
    public:
        int decompress(std::string &decompressed, const char *buf, size_t buf_size);
        int compress(std::string &compressed, const char *buf, size_t buf_size);
    };
}
