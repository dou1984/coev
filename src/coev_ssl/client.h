/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <coev/coev.h>
#include "context.h"

namespace coev::ssl
{
    struct sclient : virtual context, io_connect
    {
        sclient() = default;
        sclient(SSL_CTX *ctx);
        awaitable<int> connect(const char *host, int port) noexcept;
    };

}

namespace coev::pool::ssl
{
    struct _SSLCli : coev::ssl::sclient
    {
        template <class T>
        _SSLCli(T &conf, SSL_CTX *ctx) : sclient(ctx)
        {
            host = conf.host;
            port = conf.port;
        }
        awaitable<int> connect()
        {
            return sclient::connect(host.c_str(), port);
        }

    private:
        std::string host;
        uint16_t port;
    };

    using client = coev::client_pool<_SSLCli>;
}