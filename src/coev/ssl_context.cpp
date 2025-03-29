#include "coev.h"
#include "ssl_context.h"
#include "ssl_manager.h"

namespace coev
{
    ssl_context::ssl_context(int fd, SSL_CTX *_ssl_ctx) : io_context(fd)
    {
        m_ssl = SSL_new(_ssl_ctx);
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
            throw std::runtime_error("SSL_new failed");
        }
        int err = SSL_set_fd(m_ssl, m_fd);
        if (err != 1)
        {
            LOG_ERR("SSL_set_fd failed %d\n", err);
            __async_finally();
            throw std::runtime_error("SSL_set_mode failed");
        }
        SSL_set_accept_state(m_ssl);
    }
    ssl_context::~ssl_context()
    {
        __async_finally();
    }

    void ssl_context::__async_finally()
    {
        if (m_ssl)
        {
            SSL_free(m_ssl);
            m_ssl = nullptr;
        }
    }

    awaitable<int> ssl_context::send(const char *buf, int len)
    {
        while (__valid())
        {
            int r = SSL_write(m_ssl, buf, len);
            if (r == INVALID)
            {
                r = SSL_get_error(m_ssl, r);
                if (r != SSL_ERROR_WANT_WRITE)
                {
                    errno = -r;
                    co_return INVALID;
                }
                ev_io_start(m_loop, &m_write);
                co_await m_write_waiter.suspend();
            }
            else if (r == 0)
            {
                co_return INVALID;
            }
            else
            {
                co_return r;
            }
        }
        co_return INVALID;
    }
    awaitable<int> ssl_context::recv(char *buf, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            int r = SSL_read(m_ssl, buf, size);
            if (r == INVALID)
            {
                r = SSL_get_error(m_ssl, r);
                if (r != SSL_ERROR_WANT_READ)
                {
                    co_return INVALID;
                }
            }
            else if (r == 0)
            {
                co_return INVALID;
            }
            else
            {
                co_return r;
            }
        }
        co_return INVALID;
    }
    awaitable<int> ssl_context::do_handshake()
    {
        int err = 0;
        while ((err = SSL_do_handshake(m_ssl)) != 1)
        {
            LOG_CORE("SSL_do_handshake %d\n", err);
            err = SSL_get_error(m_ssl, err);
            if (err == SSL_ERROR_WANT_READ)
            {
                co_await m_read_waiter.suspend();
            }
            else if (err == SSL_ERROR_WANT_WRITE)
            {
                co_await m_write_waiter.suspend();
            }
            else if (err == SSL_ERROR_NONE)
            {
                if (isInprocess())
                {
                    ev_io_start(m_loop, &m_write);
                }
                co_await wait_for_any(
                    [this]() -> awaitable<void>
                    { co_await m_read_waiter.suspend(); }(),
                    [this]() -> awaitable<void>
                    { co_await m_write_waiter.suspend(); }());
            }
            else
            {
                LOG_ERR("do_handshake failed %d errno:%d %s\n", err, errno, strerror(errno));
                errno = -err;
                co_return INVALID;
            }
        }
        co_return 0;
    }

}