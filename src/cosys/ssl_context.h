#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "io_context.h"

namespace coev
{
    class ssl_context : protected io_context
    {

    public:
        using io_context::operator bool;
        ssl_context(ssl_context &&) = delete;
        ssl_context();
        ssl_context(int fd);
        ~ssl_context();

        awaitable<int> send(const char *, int);
        awaitable<int> recv(char *, int);

    public:
        awaitable<int> connect(const char *host, int port);
        static void load_certificated(const char *, const char *);
        awaitable<int> do_handshake();

    protected:
        void __async_finally();
        static int __cb_async(void *);

    protected:
        SSL *m_ssl = nullptr;
    };
}