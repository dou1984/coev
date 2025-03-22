#include "io_connect.h"

namespace coev
{  
    void io_connect::cb_connect(struct ev_loop *loop, struct ev_io *w, int revents)
    {
        if (EV_ERROR & revents)
        {
            return;
        }
        io_connect *_this = (io_connect *)(w->data);
        assert(_this != nullptr);
        _this->__del_connect();
        _this->m_read_waiter.resume(true);
    }
    void io_connect::__init_connect()
    {
        m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (m_fd == INVALID)
        {
            return;
        }
        if (setNoBlock(m_fd, true) < 0)
        {
            ::close(m_fd);
            return;
        }
        __add_connect();
    }
    io_connect::io_connect()
    {
        __init_connect();
    }
    int io_connect::__add_connect()
    {
        m_read.data = this;
        ev_io_init(&m_read, &io_connect::cb_connect, m_fd, EV_READ | EV_WRITE);
        ev_io_start(m_loop, &m_read);
        return 0;
    }
    int io_connect::__del_connect()
    {
        ev_io_stop(m_loop, &m_read);
        return 0;
    }

    int io_connect::__connect(const char *ip, int port)
    {
        if (__connect(m_fd, ip, port) < 0)
        {
            if (isInprocess())
            {
                return m_fd;
            }
        }
        return __close();
    }
    int io_connect::__connect(int fd, const char *ip, int port)
    {
        sockaddr_in addr;
        fillAddr(addr, ip, port);
        return ::connect(fd, (sockaddr *)&addr, sizeof(addr));
    }
 
    awaitable<int> io_connect::connect(const char *ip, int port)
    {
        m_fd = __connect(ip, port);
        if (m_fd == INVALID)
        {
            co_return m_fd;
        }
        co_await m_read_waiter.suspend();
        auto err = getSocketError(m_fd);
        if (err != 0)
        {
            co_return __close();
        }
        __initial();
        co_return m_fd;
    }

}