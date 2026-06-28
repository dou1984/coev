/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <array>
#include <string>
#include <cstdint>
#include <cstring>
#include "packet_encoder.h"
#include "packet_decoder.h"

namespace coev::kafka
{

    struct Uuid
    {
        std::array<uint8_t, 16> data{};
        std::string String() const;

        bool is_zero() const
        {
            for (auto b : data)
                if (b != 0)
                    return false;
            return true;
        }

        int encode(PacketEncoder &pe) const;
        int decode(PacketDecoder &pd);

        bool operator==(const Uuid &other) const { return data == other.data; }
        bool operator!=(const Uuid &other) const { return data != other.data; }
        bool operator<(const Uuid &other) const { return data < other.data; }
    };

} // namespace coev::kafka
