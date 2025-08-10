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
        virtual awaitable<int> connect(const char *, int);

    protected:
        void __init_connect();
        int __add_connect();
        int __del_connect();

        int __connect(const char *ip, int port);
        int __connect(int fd, const char *ip, int port);
        static void cb_connect(struct ev_loop *loop, struct ev_io *w, int revents);
    };
}