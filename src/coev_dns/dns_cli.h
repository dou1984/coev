#pragma once
#include <ares.h>
#include <coev/coev.h>

namespace coev
{
    class dns_cli : io_context
    {
        ares_channel m_channel;
        co_task m_task;

    public:
        dns_cli(ares_socket_t _fd, ares_channel _channel);
        ~dns_cli();
        awaitable<int> send(const char *buffer, int size);
        awaitable<int> recv(char *buffer, int size);
    };
}