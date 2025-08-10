#pragma once
#include <string>
#include <zlib.h>
#include <coev/coev.h>

namespace coev::compress::gzipZ
{

    int Compress(std::string &compressed, const char *buf, size_t buf_size);
    int Decompress(std::string &decompressd, const char *buf, size_t buf_size);

    
}
