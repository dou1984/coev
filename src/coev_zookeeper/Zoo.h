/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <coev/coev.h>
#include "ZooCli.h"

namespace coev::pool
{
    struct _Zoo : ZooCli
    {
        template <class T>
        _Zoo(T &conf)
        {
            host = conf->host;
            port = conf->port;
            username = conf->username;
            password = conf->password;
        }
        awaitable<int> connect()
        {
            return ZooCli::connect(host.c_str(), port);
        }

    private:
        std::string host;
        uint16_t port;
        std::string username;
        std::string password;
    };

    using Zoo = coev::client_pool<_Zoo>;
}