#pragma once
#include <ares.h>
#include <coev/coev.h>

namespace coev
{
    class DnsCli : io_context
    {
        ares_channel m_channel;

    public:
        DnsCli(ares_socket_t _fd, ares_channel _channel);
        awaitable<int> send(const char *buffer, int size);
        awaitable<int> recv(char *buffer, int size);
    };
}