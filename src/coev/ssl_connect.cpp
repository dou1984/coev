#include "ssl_connect.h"

namespace coev
{
    ssl_connect::ssl_connect(SSL_CTX *_ssl_ctx)
    {
        m_ssl = SSL_new(_ssl_ctx);
        if (m_ssl == nullptr)
        {
            LOG_ERR("SSL_new failed %p\n", m_ssl);
            throw std::runtime_error("SSL_new failed");
        }
    }

    awaitable<int> ssl_connect::connect(const char *host, int port)
    {
        if (m_ssl == nullptr)
        {
            throw std::runtime_error("connect error m_ssl is nullptr");
        }
        int err = co_await io_connect::connect(host, port);
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
}