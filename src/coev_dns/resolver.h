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
    class Resolver final
    {
        ares_channel m_channel;
        std::string m_ip;
        co_task m_task;
        async m_done;
        std::unordered_map<ares_socket_t, std::shared_ptr<DNSCli>> m_clients;

        std::shared_ptr<DNSCli> __find(ares_socket_t _fd);
        static void handler(void *data, ares_socket_t _fd, int readable, int writable);
        static void callback(void *arg, int status, int, struct hostent *host);

    public:
        Resolver();
        virtual ~Resolver();
        coev::awaitable<int> resolve(const std::string &hostname);
        const std::string &get_ip();
    };
}
