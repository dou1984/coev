#include "dns_cli.h"

namespace coev
{
    DnsCli::DnsCli(ares_socket_t _fd, ares_channel _channel) : io_context(_fd), m_channel(_channel)
    {
    }
    awaitable<int> DnsCli::send(const char *buffer, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            ares_process_fd(m_channel, m_fd, ARES_SOCKET_BAD);
        }
        co_return INVALID;
    }
    awaitable<int> DnsCli::recv(char *buffer, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            ares_process_fd(m_channel, ARES_SOCKET_BAD, m_fd);
        }
        co_return INVALID;
    }
}