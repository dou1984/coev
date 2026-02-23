/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <zlib.h>
#include <coev/invalid.h>
#include "coev_zlib.h"

namespace coev::zlib
{
    const size_t max_output_size = 4 * 1024;
    int Compress(std::string &compressed, const char *buf, size_t buf_size)
    {
        z_stream zs = {0};

        if (deflateInit(&zs, Z_DEFAULT_COMPRESSION) != Z_OK)
        {
            return INVALID;
        }

        zs.next_in = (z_const Bytef *)buf;
        zs.avail_in = buf_size;

        int code = 0;
        size_t output_offset = compressed.size();
        do
        {
            compressed.resize(output_offset + max_output_size);

            zs.next_out = (Bytef *)(compressed.data() + output_offset);
            zs.avail_out = max_output_size;

            code = deflate(&zs, Z_FINISH);

            size_t written = max_output_size - zs.avail_out;
            output_offset += written;
            compressed.resize(output_offset);

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
    int Decompress(std::string &decompressed, const char *buf, size_t buf_size)
    {
        z_stream zs = {0};

        if (inflateInit(&zs) != Z_OK)
        {
            return INVALID;
        }

        zs.next_in = (Bytef *)buf;
        zs.avail_in = buf_size;

        int code;
        size_t output_offset = decompressed.size();
        do
        {
            decompressed.resize(output_offset + max_output_size);
            zs.next_out = (Bytef *)(decompressed.data() + output_offset);
            zs.avail_out = max_output_size;

            code = inflate(&zs, 0);

            size_t written = max_output_size - zs.avail_out;
            output_offset += written;
            decompressed.resize(output_offset);

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
        decompressed.resize(output_offset);
        return (code == Z_STREAM_END) ? 0 : INVALID;
    }

}