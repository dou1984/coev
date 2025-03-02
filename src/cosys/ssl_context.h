#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "io_context.h"

namespace coev
{
    class ssl_context : protected io_context
    {
    public:
        ssl_context(ssl_context &&) = delete;
        ssl_context();
        ssl_context(int fd);
        ~ssl_context();

        awaitable<int> send(const char *, int);
        awaitable<int> recv(char *, int);

    public:
        static void load_certificated(const char *, const char *);

    protected:
        SSL *m_ssl;
    };
}