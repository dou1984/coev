#include <coev/coev.h>
#include "ssl_context.h"
#include "ssl_manager.h"

namespace coev
{

    ssl_context::ssl_context(SSL_CTX *_ssl_ctx)
    {
        m_ssl = SSL_new(_ssl_ctx);
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
            throw std::runtime_error("SSL_new failed");
        }
    }
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

    awaitable<int> ssl_context::connect(const char *host, int port)
    {
        if (m_ssl == nullptr)
        {
            throw std::runtime_error("connect error m_ssl is nullptr");
        }
        int err = co_await io_context::connect(host, port);
        if (err == INVALID)
        {
            LOG_ERR("connect failed %d\n", err);
        __error__:
            __async_finally();
            co_return INVALID;
        }
        err = SSL_set_fd(m_ssl, m_fd);
        if (err != 1)
        {
            LOG_ERR("SSL_set_fd failed %d\n", err);
            goto __error__;
        }
        SSL_set_connect_state(m_ssl);
        err = co_await do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake failed %d\n", err);
            goto __error__;
        }
        co_return m_fd;
    }
    awaitable<int> ssl_context::send(const char *buf, int len)
    {
        while (__valid())
        {
            if (int err = SSL_write(m_ssl, buf, len); err == INVALID)
            {
                if (err = SSL_get_error(m_ssl, err); err != SSL_ERROR_WANT_WRITE)
                {
                    errno = -err;
                    co_return INVALID;
                }
                ev_io_start(m_loop, &m_write);
                co_await m_write_waiter.suspend();
            }
            else
            {
                co_return err;
            }
        }
        co_return INVALID;
    }
    awaitable<int> ssl_context::recv(char *buf, int size)
    {
        while (__valid())
        {
            co_await m_read_waiter.suspend();
            if (auto err = SSL_read(m_ssl, buf, size); err == INVALID)
            {
                if (err = SSL_get_error(m_ssl, err); err != SSL_ERROR_WANT_READ)
                {
                    co_return INVALID;
                }
            }
            else
            {
                co_return err;
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