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
    class context : protected io_context
    {
    public:
        context(SSL_CTX *);
        context(int fd, SSL_CTX *);
        ~context();
        context(context &&) = delete;

        awaitable<int> send(const char *, int) noexcept;
        awaitable<int> recv(char *, int) noexcept;
        awaitable<int> connect(const char *host, int port) noexcept;

        awaitable<int> do_handshake();

        using io_context::close;
        using io_context::fd;
        using io_context::operator bool;
        using io_context::recvfrom;
        using io_context::sendto;

    protected:
        context() = default;
        void __async_finally();
        void __clearup();
        bool __ssl_valid() const;
        awaitable<void> __w_waiter();
        awaitable<void> __r_waiter();

    protected:
        SSL *m_ssl = nullptr;

        bool m_want_terminal = false;
    };
}