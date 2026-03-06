/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include "awaitable.h"
#include "io_context.h"
namespace coev
{
    class io_connect : virtual public io_context
    {
    public:
        using io_context::operator bool;
        io_connect();
        virtual awaitable<int> connect(const char *, int) noexcept;

    protected:
        void __init_connect() noexcept;
        int __add_connect() noexcept;
        int __del_connect() noexcept;

        int __connect(const char *ip, int port) noexcept;
        int __connect(int fd, const char *ip, int port) noexcept;
        static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents) noexcept;
    };
}