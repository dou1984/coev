#pragma once
#include <string>
#include <cstddef>
#include <lz4.h>
#include <lz4frame.h>
#include <coev/coev.h>

namespace coev::compress::lz4Z
{
    int Decompress(std::string &decompressed, const char *buf, size_t buf_size);
    int Compress(std::string &compressed, const char *buf, size_t buf_size);
}
