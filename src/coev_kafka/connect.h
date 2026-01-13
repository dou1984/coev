#pragma once
#include <errno.h>
#include <coev/coev.h>

using namespace coev;

struct Connect : io_connect
{
    using io_connect::close;
    using io_connect::connect;
    using io_connect::io_connect;

    awaitable<int> ReadFull(std::string &buf, size_t n)
    {
        assert(buf.size() == n);
        auto res = n;
        while (res > 0)
        {
            auto r = co_await recv(buf.data() + (n - res), res);
            if (r <= 0)
            {
                LOG_ERR("ReadFull failed %d %s\n", errno, strerror(errno));
                co_return INVALID;
            }

            res -= r;
        }

        co_return 0;
    }
    awaitable<int> Write(const std::string &buf)
    {
        auto r = co_await send(buf.data(), buf.size());
        if (r <= 0)
        {
            co_return INVALID;
        }
        co_return 0;
    }
};