#pragma once
#include "io_context.h"
#include "client_pool.h"

namespace coev::pool::tcp
{
    class _Connect : protected io_context
    {
    public:
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
        using io_context::send;
        using io_context::recv;
        using io_context::close;
        using io_context::fd;

    private:
        std::string host;
        uint16_t port;
    };
    using client = client_pool<_Connect>;
}