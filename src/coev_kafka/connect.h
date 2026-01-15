#pragma once
#include <errno.h>
#include <coev/coev.h>
#include "errors.h"

using namespace coev;

class Connect : protected io_connect
{

    using io_connect::io_connect;
    using base = io_connect;
    bool m_opened = false;

public:
    awaitable<int> ReadFull(std::string &buf, size_t n);
    awaitable<int> Write(const std::string &buf);
    awaitable<int> Dial(const char *addr, int port);
    int Close();
    operator bool() const;
    // Initialize the event loop pointer
    void InitLoop();
};