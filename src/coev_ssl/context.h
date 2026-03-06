/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <coev/coev.h>

namespace coev::ssl
{
    struct context : virtual io_context
    {
        context(context &&) = delete;
        context(int fd, SSL_CTX *);
        ~context();

        awaitable<int> send(const char *, int) noexcept;
        awaitable<int> recv(char *, int) noexcept;

        awaitable<int> do_handshake();

    protected:
        context() = default;
        void __async_finally();
        int __ssl_write(const char *, int);
        int __ssl_read(char *, int);

    protected:
        SSL *m_ssl = nullptr;
    };
}