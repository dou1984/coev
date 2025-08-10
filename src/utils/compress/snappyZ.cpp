#include <memory>
#include <vector>
#include <cstdint>
#include <snappy.h>
#include <snappy-c.h>
#include <coev/invalid.h>
#include "snappyZ.h"
#include "block.h"

namespace coev::compress::snappyZ
{

    int Compress(std::string &compressed, const char *buf, size_t buf_size)
    {
        if (buf == nullptr || buf_size == 0)
        {
            return INVALID;
        }
        return snappy::Compress(buf, buf_size, &compressed);
    }
    int Decompress(std::string &decompressed, const char *buf, size_t buf_size)
    {
        if (buf == nullptr || buf_size == 0)
        {
            return INVALID;
        }
        return snappy::Uncompress(buf, buf_size, &decompressed);
    }

}