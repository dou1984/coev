#pragma once
#include "ssl_context.h"

namespace coev
{
    class ssl_client : public ssl_context
    {
    public:
        ssl_client() = default;
        ~ssl_client() = default;
        awaitable<int> connect(const char *host, int port);
        
    };

}