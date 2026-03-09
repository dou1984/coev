/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <errno.h>
#include <coev/coev.h>
#include "errors.h"

namespace coev::kafka
{

    enum OpenState
    {
        CLOSED,
        OPENING,
        OPENED,
        POSTOPENED,
    };
    class Connect : protected io_context
    {
        using base = io_context;
        using base::base;
        int m_state = 0;

    public:
        Connect();
        ~Connect();
        awaitable<int> ReadFull(std::string &buf, size_t n);
        awaitable<int> Write(const std::string &buf);
        awaitable<int> Dial(const char *addr, int port);
        int Close();
        bool IsClosed() const;
        bool IsOpening() const;
        bool IsOpened() const;
        int State() const;

    public:
        co_mutex m_rlock;
        co_mutex m_wlock;
    };

} // namespace coev::kafka