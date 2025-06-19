#pragma once
#include <string>
#include <zstd.h>


namespace coev ::compress
{
    class zstdX
    {
    public:
        int compress(std::string &compressed, const char *buf, size_t n);
        int decompress(std::string &decompressd, const char *buf, size_t buf_size);
    };
}