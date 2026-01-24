#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <coev/coev.h>
#include "protocol_body.h"
#include "errors.h"

template <class Resp>
struct ResponsePromise
{
    std::chrono::time_point<std::chrono::system_clock> m_request_time;
    int32_t m_correlation_id = 0;
    Resp m_response;
    std::string m_packets;

    int decode(int16_t version)
    {
        auto err = versionedDecode(m_packets, m_response, version);
        if (err)
        {
            return ErrDecodeError;
        }
        return ErrNoError;
    }
};
