/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <ares.h>
#include <coev/coev.h>

namespace coev
{
    class DNSCli final : io_context
    {
        ares_channel m_channel;

    public:
        DNSCli(ares_socket_t _fd, ares_channel _channel);
        ~DNSCli();
        awaitable<int> send(const char *buffer, int size);
        awaitable<int> recv(char *buffer, int size);
        int close();
    };
}