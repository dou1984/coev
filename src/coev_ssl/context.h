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
        int __ssl_read(char *buffer, int size);
        bool __ssl_valid() const;
        awaitable<void> __w_waiter();
        awaitable<void> __r_waiter();

    protected:
        SSL *m_ssl = nullptr;
        struct
        {
            bool m_want_read = false;
            bool m_want_write = false;
            bool m_want_terminal = false;
        };
    };
}