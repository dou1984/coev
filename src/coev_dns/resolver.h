/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <ares.h>
#include <coev/coev.h>
#include "dns_cli.h"

namespace coev
{
    class resolver final
    {
        ares_channel m_channel;
        std::string m_ip;
        co_task m_task;
        async m_done;
        std::unordered_map<ares_socket_t, std::shared_ptr<dns_cli>> m_clients;

        std::shared_ptr<dns_cli> __find(ares_socket_t _fd);
        static void handler(void *data, ares_socket_t _fd, int readable, int writable);
        static void callback(void *arg, int status, int, struct hostent *host);

    public:
        resolver();
        virtual ~resolver();
        coev::awaitable<int> resolve(const std::string &hostname);
        std::string &get_ip();
    };
}
