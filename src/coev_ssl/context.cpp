/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <utility>
#include <coev/coev.h>
#include <utils/convert.h>
#include "context.h"
#include "manager.h"

namespace coev::ssl
{
    context::context(SSL_CTX *_ctx)
    {
        if (_ctx)
        {
            m_ssl = SSL_new(_ctx);
            if (m_ssl == nullptr)
            {
                LOG_ERR("SSL_new failed %p", m_ssl);
                throw std::runtime_error("SSL_new failed");
            }
            m_type |= IO_SSL;
        }
    }

    context::context(int fd, SSL_CTX *_ctx) : io_context(fd)
    {
        assert(m_fd != INVALID);
        if (_ctx)
        {
            m_ssl = SSL_new(_ctx);
            if (m_ssl == nullptr)
            {
                LOG_ERR("SSL_new failed %p", m_ssl);
                throw std::runtime_error("SSL_new failed");
            }
            int err = SSL_set_fd(m_ssl, m_fd);
            if (err != 1)
            {
                LOG_ERR("SSL_set_fd failed %d", err);
                __async_finally();
                throw std::runtime_error("SSL_set_mode failed");
            }
            SSL_set_accept_state(m_ssl);
            m_type |= IO_SSL;
        }
    }
    context::~context()
    {
        __clearup();
    }

    void context::__async_finally()
    {
        if (m_ssl)
        {
            m_type &= ~IO_SSL;
            auto _ssl = std::exchange(m_ssl, nullptr);
            SSL_free(_ssl);
        }
    }
    void context::__clearup()
    {
        __async_finally();
        __close();
    }
    awaitable<int> context::send(const char *buffer, int buffer_size) noexcept
    {
        if (__is_ssl() && m_ssl)
        {
            while (__ssl_valid())
            {
                int r = SSL_write(m_ssl, buffer, buffer_size);
                if (r == INVALID)
                {
                    r = SSL_get_error(m_ssl, r);
                    if (r == SSL_ERROR_WANT_WRITE)
                    {
                        if (isInprocess())
                        {
                            ev_io_start(m_loop, &m_write);
                            defer(ev_io_stop(m_loop, &m_write));
                            co_await m_w_waiter.suspend();
                        }
                        else
                        {
                            co_await m_w_waiter.suspend();
                        }
                    }
                    else if (r == SSL_ERROR_WANT_READ)
                    {
                        co_await m_r_waiter.suspend();
                    }
                    else
                    {
                        __clearup();
                        LOG_CORE("SSL_get_error %d", r);
                        errno = -r;
                        co_return INVALID;
                    }
                }
                else if (r == 0)
                {
                    co_return INVALID;
                }
                else
                {
                    LOG_CORE("write %d bytes", r);
                    co_return r;
                }
            }
            co_return INVALID;
        }
        else
        {
            co_return co_await io_context::send(buffer, buffer_size);
        }
    }
    awaitable<int> context::recv(char *buffer, int buffer_size) noexcept
    {
        if (__is_ssl() && m_ssl)
        {
            while (__ssl_valid())
            {
                int r = SSL_read(m_ssl, buffer, buffer_size);
                if (r == INVALID)
                {
                    r = SSL_get_error(m_ssl, r);
                    if (r == SSL_ERROR_WANT_READ)
                    {
                        co_await m_r_waiter.suspend();
                        if (!__ssl_valid())
                        {
                            co_return INVALID;
                        }
                    }
                    else if (r == SSL_ERROR_WANT_WRITE)
                    {
                        if (isInprocess())
                        {
                            ev_io_start(m_loop, &m_write);
                            defer(ev_io_stop(m_loop, &m_write));
                            co_await m_w_waiter.suspend();
                        }
                        else
                        {
                            co_await m_w_waiter.suspend();
                        }
                        continue;
                    }
                    else
                    {
                        __clearup();
                        LOG_CORE("SSL_get_error %d", r);
                        co_return INVALID;
                    }
                }
                else if (r == 0)
                {
                    co_return INVALID;
                }
                else
                {
                    auto data = to_hex(std::string(buffer, r));
                    LOG_CORE("read %.*s bytes", (int)data.size(), data.data());
                    co_return r;
                }
            }
            co_return INVALID;
        }
        else
        {
            co_return co_await io_context::recv(buffer, buffer_size);
        }
    }
    awaitable<int> context::connect(const char *host, int port) noexcept
    {
        int err = co_await io_context::connect(host, port);
        if (err == INVALID)
        {
            LOG_ERR("connect failed %d", err);
        __error__:
            __clearup();
            co_return INVALID;
        }
        if (m_ssl)
        {
            err = SSL_set_fd(m_ssl, m_fd);
            if (err != 1)
            {
                LOG_ERR("SSL_set_fd failed %d", err);
                goto __error__;
            }
            SSL_set_connect_state(m_ssl);
            err = co_await do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake failed %d", err);
                goto __error__;
            }
        }
        co_return m_fd;
    }
    awaitable<int> context::do_handshake()
    {
        if (__is_ssl())
        {
            int err = 0;
            while (__ssl_valid() && (err = SSL_do_handshake(m_ssl)) != 1)
            {
                LOG_CORE("SSL_do_handshake %d", err);
                err = SSL_get_error(m_ssl, err);
                if (err == SSL_ERROR_WANT_READ)
                {
                    co_await m_r_waiter.suspend();
                    continue;
                }
                else if (err == SSL_ERROR_WANT_WRITE)
                {
                    if (isInprocess())
                    {
                        ev_io_start(m_loop, &m_write);
                        defer(ev_io_stop(m_loop, &m_write));
                        co_await m_w_waiter.suspend();
                    }
                    else
                    {
                        co_await m_w_waiter.suspend();
                    }
                    continue;
                }
                else if (err == SSL_ERROR_NONE)
                {
                    // 握手成功，直接返回
                    break;
                }
                else
                {
                    LOG_ERR("do_handshake failed %d errno:%d %s", err, errno, strerror(errno));
                    errno = -err;
                    co_return INVALID;
                }
            }
        }
        co_return 0;
    }

    int context::__ssl_write(const char *buffer, int buffer_size)
    {
        if (!__ssl_valid())
        {
            return INVALID;
        }
        if (buffer_size <= 0)
        {
            return INVALID;
        }
        std::string data = to_hex(std::string(buffer, buffer_size));
        LOG_CORE("send %.*s", (int)data.size(), data.data());
        int r = SSL_write(m_ssl, buffer, buffer_size);
        if (r == INVALID)
        {
            r = SSL_get_error(m_ssl, r);
            if (r != SSL_ERROR_WANT_WRITE)
            {
                LOG_CORE("SSL_write error %d", r);
                errno = -r;
                return INVALID;
            }
            return INVALID;
        }
        else if (r == 0)
        {
            errno = 0;
            return INVALID;
        }
        return r;
    }
    bool context::__ssl_valid() const
    {
        return __valid() && (!__is_ssl() || m_ssl);
    }
}