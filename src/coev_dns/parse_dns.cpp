/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <ares.h>
#include <memory>
#include <coev/coev.h>
#include "resolver.h"
#include "dns_cli.h"

namespace coev
{
    coev::awaitable<int> parse_dns(const std::string &url, std::string &addr)
    {
        try
        {
            resolver _resolver;
            int r = co_await _resolver.resolve(url);
            if (r == 0)
            {
                addr = _resolver.get_ip();
                LOG_CORE("resolver %s\n", addr.c_str());
                co_return 0;
            }
            co_return r;
        }
        catch (std::exception &e)
        {
            LOG_ERR("parse_dns error %s\n", e.what());
        }
        co_return INVALID;
    }
}