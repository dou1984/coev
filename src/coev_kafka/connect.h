#pragma once
#include <errno.h>
#include <coev/coev.h>

using namespace coev;

struct Connect : io_connect
{
    using io_connect::close;
    using io_connect::connect;
    using io_connect::io_connect;

    coev::awaitable<int> ReadFull(std::string &buf, size_t &n)
    {
        n = co_await recv((char *)buf.data(), buf.size());
        if (n < 0)
        {
            co_return errno;
        }
        co_return 0;
    }
    coev::awaitable<int> Write(const std::string &buf, size_t &n)
    {
        n = co_await send((const char *)buf.data(), buf.size());
        if (n < 0)
        {
            co_return errno;
        }
        co_return 0;
    }
};