#pragma once
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <coev/coev.h>

namespace coev::ssl
{
    class context : virtual public io_context
    {
    public:
        context(context &&) = delete;
        context(int fd, SSL_CTX *);
        ~context();

        awaitable<int> send(const char *, int);
        awaitable<int> recv(char *, int);

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