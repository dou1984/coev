/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <iostream>
#include <zstd.h>
#include <coev/invalid.h>
#include "coev_zstd.h"
#include "block.h"

namespace coev::zstd
{
    int Compress(std::string &compressed, const char *buf, size_t buf_size)
    {
        if (buf == nullptr || buf_size == 0)
        {
            return INVALID;
        }

        ZSTD_CCtx *ctx = ZSTD_createCCtx();
        if (!ctx)
        {
            return INVALID;
        }

        size_t max_compressed_size = ZSTD_compressBound(buf_size);

        size_t output_offset = compressed.size();
        compressed.resize(output_offset + sizeof(uint32_t) + max_compressed_size);

        details::block __this_block = {
            .str = compressed.data() + output_offset,
        };
        __this_block.set_size(buf_size);

        ZSTD_inBuffer input =
            {
                .src = buf,
                .size = buf_size,
                .pos = 0,
            };
        ZSTD_outBuffer output =
            {
                .dst = __this_block.body(),
                .size = max_compressed_size,
                .pos = 0,
            };

        auto code = ZSTD_compressStream(ctx, &output, &input);
        if (ZSTD_isError(code) || input.pos < input.size)
        {
            ZSTD_freeCCtx(ctx);
            return INVALID;
        }

        code = ZSTD_endStream(ctx, &output);
        if (ZSTD_isError(code))
        {
            ZSTD_freeCCtx(ctx);
            return INVALID;
        }
        ZSTD_freeCCtx(ctx);

        compressed.resize(output_offset + sizeof(uint32_t) + output.pos);
        return 0;
    }

    int Decompress(std::string &decompressed, const char *buf, size_t buf_size)
    {
        if (buf == nullptr || buf_size == 0)
        {
            return INVALID;
        }

        const char *begin = buf;
        const char *end = buf + buf_size;

        ZSTD_DCtx *ctx = ZSTD_createDCtx();
        if (!ctx)
        {
            return INVALID;
        }

        while (begin < end)
        {

            details::block __this_block =
                {
                    .c_str = begin,
                };
            if (begin + sizeof(uint32_t) > end)
            {
                ZSTD_freeDCtx(ctx);
                return INVALID;
            }

            uint64_t output_len = __this_block.size();
            begin += sizeof(uint32_t);

            size_t output_offset = decompressed.size();
            decompressed.resize(output_offset + output_len);

            ZSTD_inBuffer input =
                {
                    .src = begin,
                    .size = static_cast<size_t>(end - begin),
                    .pos = 0,
                };
            ZSTD_outBuffer output =
                {
                    .dst = decompressed.data() + output_offset,
                    .size = output_len,
                    .pos = 0,
                };

            while (output.pos < output_len)
            {
                size_t code = ZSTD_decompressStream(ctx, &output, &input);
                if (ZSTD_isError(code))
                {
                    ZSTD_freeDCtx(ctx);
                    return INVALID;
                }
                if (input.pos == input.size && output.pos < output_len)
                {
                    ZSTD_freeDCtx(ctx);
                    return INVALID;
                }
            }
            begin += input.pos;
        }

        ZSTD_freeDCtx(ctx);
        return 0;
    }
}