/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include "client.h"

namespace coev::ssl
{
    client::client(SSL_CTX *_ssl_ctx)
    {
        if (_ssl_ctx)
        {
            m_ssl = SSL_new(_ssl_ctx);
            if (m_ssl == nullptr)
            {
                LOG_ERR("SSL_new failed %p\n", m_ssl);
                throw std::runtime_error("SSL_new failed");
            }
            m_type |= IO_SSL;
        }
    }

    awaitable<int> client::connect(const char *host, int port)
    {

        int err = co_await coev::io_connect::connect(host, port);
        if (err == INVALID)
        {
            LOG_ERR("connect failed %d\n", err);
        __error__:
            __async_finally();
            co_return INVALID;
        }
        if (m_ssl)
        {
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
        }
        co_return m_fd;
    }
}