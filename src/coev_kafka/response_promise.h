/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <coev/coev.h>
#include "protocol_body.h"
#include "errors.h"

namespace coev::kafka
{
    template <class Res>
    struct ResponsePromise
    {
        std::chrono::time_point<std::chrono::system_clock> m_request_time;
        int32_t m_correlation_id = 0;
        std::string m_packets;
        std::shared_ptr<Res> m_response = std::make_shared<Res>();

        ResponsePromise() = default;

        int decode(int16_t version)
        {
            auto err = decode_version(m_packets, *m_response, version);
            if (err)
            {
                return ErrDecodeError;
            }
            return ErrNoError;
        }
    };
}