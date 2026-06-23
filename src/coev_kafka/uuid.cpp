/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "uuid.h"
#include "packet_encoder.h"
#include "packet_decoder.h"

namespace coev::kafka
{
    static std::string base64UrlEncodeWithoutPadding(const uint8_t *data, size_t len)
    {
        static const char kBase64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
        std::string result;
        size_t i = 0;
        uint32_t triple;

        while (i < len)
        {
            triple = (static_cast<uint32_t>(data[i]) << 16);
            if (i + 1 < len)
                triple |= (static_cast<uint32_t>(data[i + 1]) << 8);
            if (i + 2 < len)
                triple |= static_cast<uint32_t>(data[i + 2]);

            result.push_back(kBase64Chars[(triple >> 18) & 0x3F]);
            result.push_back(kBase64Chars[(triple >> 12) & 0x3F]);
            if (i + 1 < len)
            {
                result.push_back(kBase64Chars[(triple >> 6) & 0x3F]);
            }
            if (i + 2 < len)
            {
                result.push_back(kBase64Chars[triple & 0x3F]);
            }
            i += 3;
        }
        return result;
    }

    std::string Uuid::String() const
    {
        return base64UrlEncodeWithoutPadding(data.data(), data.size());
    }

    int Uuid::encode(packet_encoder &pe) const
    {
        return pe.putRawBytes(std::string_view(reinterpret_cast<const char *>(data.data()), 16));
    }

    int Uuid::decode(packet_decoder &pd)
    {
        std::string_view raw;
        int err = pd.getRawBytes(16, raw);
        if (err != 0)
            return err;
        std::memcpy(data.data(), raw.data(), 16);
        return 0;
    }
}
