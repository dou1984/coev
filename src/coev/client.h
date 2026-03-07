#pragma once
#include "io_context.h"
#include "client_pool.h"

namespace coev::pool::tcp
{
    struct _Connect : io_context
    {
        template <class T>
        _Connect(T &conf)
        {
            host = conf->host;
            port = conf->port;
        }
        awaitable<int> connect()
        {
            return io_context::connect(host.c_str(), port);
        }
        using io_context::operator bool;

    private:
        std::string host;
        uint16_t port;
    };
    using client = client_pool<_Connect>;
}