/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "dns_cli.h"

namespace coev
{
    DNSCli::DNSCli(ares_socket_t _fd, ares_channel _channel) : io_context(_fd), m_channel(_channel)
    {
        m_type = IO_CLI;
    }
    DNSCli::~DNSCli()
    {
        // 是否要置为INVALID?
        // m_fd = INVALID;
    }
    int DNSCli::close()
    {
        return io_context::close();
    }
    awaitable<int> DNSCli::send(const char *buffer, int size)
    {
        while (__valid())
        {
            co_await m_r_waiter.suspend();
            ares_process_fd(m_channel, ARES_SOCKET_BAD, m_fd);
        }
        co_return INVALID;
    }
    awaitable<int> DNSCli::recv(char *buffer, int size)
    {
        while (__valid())
        {
            co_await m_r_waiter.suspend();
            ares_process_fd(m_channel, m_fd, ARES_SOCKET_BAD);
        }
        co_return INVALID;
    }
}