#include <zlib.h>
#include <coev/invalid.h>
#include "zlibX.h"

namespace coev::compress
{
    const size_t max_output_size = 4 * 1024;
    int zlibX::compress(std::string &compressed, const char *buf, size_t buf_size)
    {
        z_stream zs = {0};

        if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
        {
            return INVALID;
        }

        zs.next_in = (z_const Bytef *)buf;
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
    int zlibX::decompress(std::string &decompressed, const char *buf, size_t buf_size)
    {
        z_stream zs = {0};

        if (inflateInit(&zs) != Z_OK)
        {
            return INVALID;
        }

        zs.next_in = (Bytef *)buf;
        zs.avail_in = buf_size;

        int code;
        size_t total_written = 0;
        do
        {
            decompressed.resize(total_written + max_output_size);
            zs.next_out = (Bytef *)(decompressed.data() + total_written);
            zs.avail_out = max_output_size;

            code = inflate(&zs, 0);

            size_t written = max_output_size - zs.avail_out;
            total_written += written;
            decompressed.resize(total_written);

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
        decompressed.resize(total_written);
        return (code == Z_STREAM_END) ? 0 : INVALID;
    }

}