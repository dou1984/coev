/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <coev/coev.h>
#include <coev_ssl/ssl.h>
#include "session.h"

namespace coev::pool::nghttp2
{
    struct _Nghttp : coev::nghttp2::session
    {
        template <typename T>
        _Nghttp(T &conf) : coev::nghttp2::session((SSL_CTX *)conf->data)
        {
            m_host = conf->host;
            m_port = conf->port;
        }
        awaitable<int> connect();

    private:
        std::string m_host;
        int m_port;
    };

    using Http2 = coev::client_pool<_Nghttp>;
}