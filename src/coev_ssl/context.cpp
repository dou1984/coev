#include <coev/coev.h>
#include "context.h"
#include "manager.h"

namespace coev::ssl
{

    context::context(int fd, SSL_CTX *_ssl_ctx) : io_context(fd)
    {
        assert(m_fd != INVALID);
        if (_ssl_ctx)
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
            m_type |= IO_SSL;
        }
    }
    context::~context()
    {
        __async_finally();
    }

    void context::__async_finally()
    {
        if (m_ssl)
        {
            SSL_free(m_ssl);
            m_ssl = nullptr;
        }
    }

    awaitable<int> context::send(const char *buf, int len)
    {
        if (!__is_ssl())
        {
            auto r = co_await io_context::send(buf, len);
            co_return r;
        }
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
    awaitable<int> context::recv(char *buf, int size)
    {
        if (!__is_ssl())
        {
            auto r = co_await io_context::recv(buf, size);
            co_return r;
        }
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            if (__invalid())
            {
                co_return INVALID;
            }
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
    awaitable<int> context::do_handshake()
    {
        if (!__is_ssl())
        {
            co_return 0;
        }
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
    int context::__ssl_write(const char *buffer, int buffer_size)
    {

        assert(m_ssl);
        int r = SSL_write(m_ssl, buffer, buffer_size);
        if (r == INVALID)
        {
            r = SSL_get_error(m_ssl, r);
            if (r != SSL_ERROR_WANT_WRITE)
            {
                errno = -r;
                return INVALID;
            }
            ev_io_start(m_loop, &m_write);
        }
        else if (r == 0)
        {
            errno = 0;
            return INVALID;
        }
        return r;
    }
    int context::__ssl_read(char *buffer, int buffer_size)
    {
        assert(m_ssl);
        int r = SSL_read(m_ssl, buffer, buffer_size);
        if (r == INVALID)
        {
            r = SSL_get_error(m_ssl, r);
            if (r != SSL_ERROR_WANT_READ)
            {
                return INVALID;
            }
        }
        else if (r == 0)
        {
            errno = 0;
            return INVALID;
        }
        return r;
    }
}