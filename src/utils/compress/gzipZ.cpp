#include <zlib.h>
#include <coev/invalid.h>
#include "gzipZ.h"

namespace coev::compress::gzipZ
{
    const size_t max_output_size = 16 * 1024;
    int Compress(std::string &compressed, const char *buf, size_t buf_size)
    {
        z_stream zs = {0};

        if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            return INVALID;
        }
        zs.next_in = (Bytef *)buf;
        zs.avail_in = buf_size;

        int code = 0;
        size_t total_written = 0;
        do
        {
            compressed.resize(total_written + max_output_size);
            zs.next_out = (Bytef *)(compressed.data() + total_written);
            zs.avail_out = max_output_size;

            code = deflate(&zs, Z_FINISH);

            size_t written = max_output_size - zs.avail_out;
            total_written += written;
            compressed.resize(total_written);

            if (code == Z_STREAM_END)
            {
                break;
            }
            if (code != Z_OK && code != Z_BUF_ERROR)
            {
                deflateEnd(&zs);
                return INVALID;
            }
        } while (zs.avail_out == 0);

        deflateEnd(&zs);

        return (code == Z_STREAM_END) ? 0 : INVALID;
    }
    int Decompress(std::string &decompressd, const char *buf, size_t buf_size)
    {
        z_stream zs = {0};

        if (inflateInit2(&zs, 15 | 16) != Z_OK)
        {
            return INVALID;
        }
        zs.next_in = (Bytef *)buf;
        zs.avail_in = buf_size;

        int code = 0;
        size_t total_written = 0;
        do
        {
            decompressd.resize(total_written + max_output_size);
            zs.next_out = (Bytef *)(decompressd.data() + total_written);
            zs.avail_out = max_output_size;

            code = inflate(&zs, Z_NO_FLUSH);
            size_t written = max_output_size - zs.avail_out;
            total_written += written;
            decompressd.resize(total_written);

            if (code == Z_STREAM_END)
            {
                break;
            }
            if (code != Z_OK && code != Z_BUF_ERROR)
            {
                inflateEnd(&zs);
                return INVALID;
            }
        } while (zs.avail_out == 0);

        inflateEnd(&zs);

        return (code == Z_STREAM_END) ? 0 : INVALID;
    }

}