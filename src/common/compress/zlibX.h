#pragma once
#include <string>


namespace coev::compress
{
    class zlibX
    {
    public:
        int compress(std::string &compressed, const char *buf, size_t buf_size);
        int decompress(std::string &decompressd, const char *buf, size_t buf_size);
    };
}