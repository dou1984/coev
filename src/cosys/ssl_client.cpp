#include "ssl_client.h"

namespace coev
{

    awaitable<int> ssl_client::connect(const char *host, int port)
    {

        int err = co_await io_context::connect(host, port);
        if (err == INVALID)
        {
            LOG_ERR("connect failed %d\n", err);
            co_return INVALID;
        }

        SSL_set_fd(m_ssl, m_fd);
        err = SSL_connect(m_ssl);
        if (err == INVALID)
        {
            LOG_ERR("connect failed %d\n", err);
            co_return INVALID;
        }
        co_return 0;
    }
}