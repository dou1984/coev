#pragma once
#include <coev/coev.h>
#include "ssl_context.h"

namespace coev::ssl
{
    class ssl_client : virtual public ssl_context, protected coev::io_connect
    {
    public:
        ssl_client() = default;
        ssl_client(SSL_CTX *ctx);
        awaitable<int> connect(const char *host, int port);
    };

}