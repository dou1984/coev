#pragma once
#include "awaitable.h"
#include "io_connect.h"
#include "ssl_context.h"

namespace coev
{
    class ssl_connect : public ssl_context, protected io_connect
    {

    public:
        ssl_connect(SSL_CTX *ctx);
        awaitable<int> connect(const char *host, int port);
    };
}