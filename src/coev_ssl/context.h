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
    struct context : io_context
    {
        context(SSL_CTX *);
        context(int fd, SSL_CTX *);
        ~context();
        context(context &&) = delete;

        awaitable<int> send(const char *, int) noexcept;
        awaitable<int> recv(char *, int) noexcept;
        awaitable<int> connect(const char *host, int port) noexcept;

        awaitable<int> do_handshake();

    protected:
        context() = default;
        void __async_finally();
        void __clearup();
        int __ssl_write(const char *, int);
        int __ssl_read(char *, int);
        bool __ssl_valid() const { return __valid() && m_ssl; }

    protected:
        SSL *m_ssl = nullptr;
    };
}