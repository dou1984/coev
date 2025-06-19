#pragma once
#include <coev/coev.h>
#include "context.h"

namespace coev::ssl
{
    class client : virtual public context, protected io_connect
    {
    public:
        client() = default;
        client(SSL_CTX *ctx);
        awaitable<int> connect(const char *host, int port);
    };
}