/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
// timestamp.h
#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <coev/coev.h>
#include "packet_encoder.h"
#include "packet_decoder.h"

namespace coev::kafka
{
    struct Timestamp
    {

        Timestamp();
        Timestamp(const std::chrono::system_clock::time_point &t);

        int encode(packet_encoder &pe) const;
        int decode(packet_decoder &pd);

        std::chrono::system_clock::time_point get_time() const;
        void set_time(const std::chrono::system_clock::time_point &t);

        std::chrono::system_clock::time_point m_time;
    };
}