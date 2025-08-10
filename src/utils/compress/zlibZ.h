#pragma once
#include <string>

namespace coev::compress::zlibX
{

    int Compress(std::string &compressed, const char *buf, size_t buf_size);
    int Decompress(std::string &decompressd, const char *buf, size_t buf_size);

}