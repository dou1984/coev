#include "NghttpConnect.h"

namespace coev::nghttp2
{
    NghttpConnect::NghttpConnect(SSL_CTX *_ssl_ctx) : ssl_connect(_ssl_ctx)
    {
    }
    awaitable<int> NghttpConnect::connect(const char *url)
    {
        if (m_fd == INVALID)
        {
            co_return m_fd;
        }
        addrInfo info;
        int err = info.fromUrl(url);
        if (err == INVALID)
        {
        __error_return__:
            ::close(m_fd);
            m_fd = INVALID;
            co_return m_fd;
        }
        err = co_await ssl_connect::connect(info.ip, info.port);
        if (err == INVALID)
        {
            LOG_ERR("ssl connect failed %s %d\n", info.ip, info.port);
            goto __error_return__;
        }
        err = nghttp2_session_client_new(&m_session, m_callbacks, this);
        if (err < 0)
        {
            LOG_ERR("nghttp2_session_client_new failed %s %d\n", info.ip, info.port);
            goto __error_return__;
        }

        err = nghttp2_session_send(m_session);
        if (err < 0)
        {
            LOG_ERR("nghttp2_session_send failed %s %d\n", info.ip, info.port);
            goto __error_return__;
        }
        co_return m_fd;
    }
   
}