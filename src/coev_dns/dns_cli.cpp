#include "dns_cli.h"

namespace coev
{
    dns_cli::dns_cli(ares_socket_t _fd, ares_channel _channel) : io_context(_fd), m_channel(_channel)
    {
    }
    dns_cli::~dns_cli()
    {
        // 是否要置为INVALID?
        // m_fd = INVALID;
    }
    awaitable<int> dns_cli::send(const char *buffer, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            ares_process_fd(m_channel, ARES_SOCKET_BAD, m_fd);
        }
        co_return INVALID;
    }
    awaitable<int> dns_cli::recv(char *buffer, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            ares_process_fd(m_channel, m_fd, ARES_SOCKET_BAD);
        }
        co_return INVALID;
    }
}