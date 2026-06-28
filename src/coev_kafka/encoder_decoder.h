/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#pragma once
#include <vector>
#include <memory>
#include <cstdint>
#include <cassert>
#include <string>
#include <string_view>

namespace coev::kafka
{
    struct PacketDecoder;
    struct PacketEncoder;

    struct IEncoder
    {
        virtual ~IEncoder() = default;
        virtual int encode(PacketEncoder &pe) const = 0;
    };

    struct HEncoder : IEncoder
    {
        virtual int16_t header_version() const = 0;
    };
    struct VEncoder
    {
        virtual ~VEncoder() = default;
        virtual int encode(PacketEncoder &pe, int16_t version) const = 0;
    };

    struct IDecoder
    {
        virtual ~IDecoder() = default;
        virtual int decode(PacketDecoder &pd) = 0;
    };

    struct VDecoder
    {
        virtual ~VDecoder() = default;
        virtual int decode(PacketDecoder &pd, int16_t version) = 0;
    };

    int encode(const IEncoder &e, std::string &out);
    int decode(const std::string &buf, IDecoder &in);
    int decode_version(std::string_view buf, VDecoder &in, int16_t version);
    int magic_value(PacketDecoder &pd, int8_t &magic);

    int prepare_flexible_decoder(PacketDecoder &pd, VDecoder &req, int16_t version);
    int prepare_flexible_encoder(PacketEncoder &pe, const IEncoder &req);

}