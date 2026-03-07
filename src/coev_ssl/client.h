/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <coev/coev.h>
#include "context.h"

namespace coev::pool::ssl
{
    struct _SSLCli : coev::ssl::context
    {
        template <class T>
        _SSLCli(T &conf, SSL_CTX *ctx) : coev::ssl::context(ctx)
        {
            host = conf.host;
            port = conf.port;
        }
        awaitable<int> connect() noexcept
        {
            return coev::ssl::context::connect(host.c_str(), port);
        }

    private:
        std::string host;
        uint16_t port;
    };

    using client = coev::client_pool<_SSLCli>;
}