#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "io_context.h"

namespace coev
{
    class ssl_context : virtual protected io_context
    {
    protected:
        ssl_context(SSL_CTX *);

    public:
        using io_context::operator bool;
        using io_context::close;
        ssl_context(ssl_context &&) = delete;

        ssl_context(int fd, SSL_CTX *);
        ~ssl_context();

        awaitable<int> send(const char *, int);
        awaitable<int> recv(char *, int);

        awaitable<int> do_handshake();

    protected:
        void __async_finally();

    protected:
        SSL *m_ssl = nullptr;
    };
}