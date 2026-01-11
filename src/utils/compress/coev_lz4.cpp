#include <lz4.h>
#include <lz4frame.h>
#include <stdexcept>
#include <coev/invalid.h>
#include "coev_lz4.h"
#include "block.h"
namespace coev::lz4
{

    int Compress(std::string &compressed, const char *buf, size_t buf_size)
    {

        LZ4F_compressionContext_t ctx = nullptr;
        auto code = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
        if (LZ4F_isError(code))
        {
            return INVALID;
        }
        auto output_offset = compressed.size();
        auto max_compressed_size = LZ4F_compressBound(buf_size, nullptr);

        compressed.resize(output_offset + max_compressed_size);
        auto compressed_size = LZ4F_compressFrame(compressed.data() + output_offset, max_compressed_size, buf, buf_size, nullptr);
        if (LZ4F_isError(compressed_size))
        {
            LZ4F_freeCompressionContext(ctx);
            return INVALID;
        }
        compressed.resize(output_offset + compressed_size);
        LZ4F_freeCompressionContext(ctx);
        return 0;
    }
    int Decompress(std::string &decompressed, const char *buf, size_t buf_size)
    {
        if (buf == nullptr || buf_size == 0)
        {
            return INVALID;
        }

        LZ4F_decompressionContext_t ctx = nullptr;
        auto code = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
        if (LZ4F_isError(code))
        {
            LZ4F_freeDecompressionContext(ctx);
            return INVALID;
        }

        LZ4F_frameInfo_t info = {};
        auto input_offset = buf_size;
        code = LZ4F_getFrameInfo(ctx, &info, buf, &input_offset);
        if (LZ4F_isError(code))
        {
            LZ4F_freeDecompressionContext(ctx);
            return INVALID;
        }
        size_t output_offset = decompressed.size();
        auto max_decompressed_size = info.contentSize == 0 ? 4 * 1024 : info.contentSize;
        decompressed.resize(output_offset + max_decompressed_size);

        while (true)
        {
            size_t output_res = decompressed.size() - output_offset;
            size_t input_res = buf_size - input_offset;

            if (output_res < 1024)
            {
                decompressed.resize(decompressed.size() * 2);
                output_res = decompressed.size() - output_offset;
            }

            code = LZ4F_decompress(ctx, decompressed.data() + output_offset, &output_res, buf + input_offset, &input_res, nullptr);
            if (LZ4F_isError(code))
            {
                LZ4F_freeDecompressionContext(ctx);
                return INVALID;
            }

            input_offset += input_res;
            output_offset += output_res;

            if (code == 0)
            {
                decompressed.resize(output_offset);
                break;
            }
        }

        LZ4F_freeDecompressionContext(ctx);
        return 0;
    }

}