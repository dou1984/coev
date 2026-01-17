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

    std::function<void(std::string &, KError)> m_handler;
    coev::co_channel<std::string> m_packets;
    coev::co_channel<KError> m_errors;
    void Handle(std::string &packets_, KError err)
    {
        if (m_handler)
        {
            m_handler(packets_, err);
            return;
        }
        if (err)
        {
            m_errors.set(err);
            return;
        }

        m_packets.set(std::move(packets_));
    }
};

template <class Req, class Resp>
coev::awaitable<int> HandleResponsePromise(const Req &req, Resp &res, ResponsePromise<Resp> &promise)
{
    auto packets = co_await promise.m_packets.get();
    if (packets.empty())
    {
        auto err = co_await promise.m_errors.get();
        co_return err;
    }
    co_return versionedDecode(packets, res, req.version());
}