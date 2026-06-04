/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_dns/parse_dns.h>
#include <coev/log.h>

using namespace coev;

std::string g_url;
awaitable<void> co_parse_dns()
{
    // std::string url = "www.baidu.com";
    std::string url = g_url;
    std::string addr = "";
    auto r = co_await parse_dns(url, addr);
    if (r != 0)
    {
        LOG_ERR("parse_dns error ");
        co_return;
    }
    LOG_INFO("%s -> %s", url.c_str(), addr.c_str());

    co_return;
}

int main(int argc, char **argv)
{
    if (argc > 1)
    {
        g_url = argv[1];
    }
    else
    {
        g_url = "www.baidu.com";
    }

    set_log_level(LOG_LEVEL_CORE);

    runnable::instance()
        .start(co_parse_dns)
        .end();

    return 0;
}