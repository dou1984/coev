/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <sys/ioctl.h>
#include <linux/sockios.h>
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
                LOG_ERR("fd %d SSL_new failed %p", m_fd, m_ssl);
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
                LOG_ERR("fd %d SSL_new failed %p", m_fd, m_ssl);
                throw std::runtime_error("SSL_new failed");
            }
            int err = SSL_set_fd(m_ssl, m_fd);
            if (err != 1)
            {
                LOG_ERR("fd %d SSL_set_fd failed %d", m_fd, err);
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
            auto _ssl = std::exchange(m_ssl, nullptr);
            SSL_free(_ssl);
            m_type &= ~IO_SSL;
        }
    }
    void context::__clearup()
    {
        __async_finally();
        __close();
    }
    awaitable<int> context::send(const char *buffer, int buffer_size) noexcept
    {
        if (buffer_size <= 0)
        {
            co_return INVALID;
        }

        if (!__is_ssl())
        {
            assert(m_ssl == nullptr);
            co_return co_await io_context::send(buffer, buffer_size);
        }

        if (!__ssl_valid())
        {
            co_return INVALID;
        }

        int send_offset = 0;
        while (send_offset < buffer_size)
        {
            int r = SSL_write(m_ssl, buffer + send_offset, buffer_size - send_offset);
            if (r > 0)
            {
                send_offset += r;
                continue;
            }

            r = SSL_get_error(m_ssl, r);
            if (r == SSL_ERROR_WANT_WRITE)
            {
                co_await __w_waiter();
            }
            else if (r == SSL_ERROR_WANT_READ)
            {
                co_await __r_waiter();
            }
            else
            {
                errno = -r;
                co_return INVALID;
            }
        }

        co_return send_offset;
    }
    awaitable<int> context::recv(char *buffer, int buffer_size) noexcept
    {
        if (m_ssl == nullptr)
        {
            co_return co_await io_context::recv(buffer, buffer_size);
        }

        if (!__ssl_valid())
        {
            co_return INVALID;
        }

        while (true)
        {
            int r = SSL_read(m_ssl, buffer, buffer_size);
            if (r > 0)
            {
                co_return r;
            }

            r = SSL_get_error(m_ssl, r);
            if (r == SSL_ERROR_WANT_READ)
            {
                int bytes_available = 0;
                if (ioctl(m_fd, FIONREAD, &bytes_available) == 0 && bytes_available > 0)
                {
                    continue;
                }
                co_await __r_waiter();
            }
            else if (r == SSL_ERROR_WANT_WRITE)
            {
                co_await __w_waiter();
            }
            else if (r == SSL_ERROR_ZERO_RETURN)
            {
                co_return 0;
            }
            else if (r == SSL_ERROR_SSL)
            {
                errno = ECONNRESET;
                co_return INVALID;
            }
            else
            {
                errno = ECONNRESET;
                co_return INVALID;
            }
        }
    }
    awaitable<int> context::connect(const char *host, int port) noexcept
    {
        int err = co_await io_context::connect(host, port);
        if (err == INVALID)
        {
            LOG_ERR("connect fd %d failed %d", m_fd, err);
        __error__:
            __clearup();
            co_return INVALID;
        }
        if (m_ssl)
        {
            err = SSL_set_fd(m_ssl, m_fd);
            if (err != 1)
            {
                LOG_ERR("SSL_set_fd fd %d failed %d", m_fd, err);
                goto __error__;
            }
            SSL_set_connect_state(m_ssl);
            err = co_await do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake fd %d failed %d", m_fd, err);
                goto __error__;
            }
            LOG_INFO("fd %d handshake success", m_fd);
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
                err = SSL_get_error(m_ssl, err);
                if (err == SSL_ERROR_WANT_READ)
                {
                    co_await __r_waiter();
                }
                else if (err == SSL_ERROR_WANT_WRITE)
                {
                    co_await __w_waiter();
                }
                else if (err == SSL_ERROR_NONE)
                {
                    break;
                }
                else
                {
                    char errbuf[256];
                    unsigned long ssl_err_code = ERR_get_error();
                    ERR_error_string_n(ssl_err_code, errbuf, sizeof(errbuf));
                    LOG_ERR("do_handshake fd %d failed ssl_err:%d openssl_err:%lu %s", m_fd, err, ssl_err_code, errbuf);
                    errno = -err;
                    co_return INVALID;
                }
            }
        }
        co_return 0;
    }

    bool context::__ssl_valid() const
    {
        return __valid() && (!__is_ssl() || m_ssl);
    }
    awaitable<void> context::__w_waiter()
    {
        int send_buffer_size = 0;
        socklen_t len = sizeof(send_buffer_size);
        if (getsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &send_buffer_size, &len) == 0)
        {
            int bytes_queued = 0;
            if (ioctl(m_fd, TIOCOUTQ, &bytes_queued) == 0)
            {
                if (bytes_queued < send_buffer_size)
                {
                    co_return;
                }
            }
        }
        if (isInprocess())
        {
            co_await m_w_waiter.suspend();
        }
    }
    awaitable<void> context::__r_waiter()
    {
        int bytes_available = 0;
        if (ioctl(m_fd, FIONREAD, &bytes_available) == 0 && bytes_available > 0)
        {
            co_return;
        }
        if (isInprocess())
        {
            co_await m_r_waiter.suspend();
        }
    }
}