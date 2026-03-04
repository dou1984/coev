#pragma once
#include "io_connect.h"
#include "client_pool.h"

namespace coev::pool::tcp
{
    struct _Connect : io_connect
    {
        template <class T>
        _Connect(T &conf)
        {
            host = conf->host;
            port = conf->port;
        }
        awaitable<int> connect()
        {
            return io_connect::connect(host.c_str(), port);
        }
        using io_connect::operator bool;

    private:
        std::string host;
        uint16_t port;
    };
    using client = client_pool<_Connect>;
}