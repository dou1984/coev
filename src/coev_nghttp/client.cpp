#include "client.h"

namespace coev::nghttp2
{
    client::client(SSL_CTX *_ssl_ctx) : ssl::client(_ssl_ctx)
    {
        m_type |= IO_CLI;
        if (_ssl_ctx)
        {
            m_type |= IO_SSL;
        }
    }
    awaitable<int> client::connect(const char *url)
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
            __close();
            co_return m_fd;
        }
        err = co_await ssl::client::connect(info.ip, info.port);
        if (err == INVALID)
        {
            LOG_ERR("ssl connect failed %s %d %s", info.ip, info.port, nghttp2_strerror(err));
            goto __error_return__;
        }
        assert(m_ssl);

        auto _this = static_cast<session *>(this);
        err = nghttp2_session_client_new(&m_session, m_callbacks, _this);
        if (err != 0)
        {
            LOG_ERR("nghttp2_session_client_new failed %s %d %s", info.ip, info.port, nghttp2_strerror(err));
            goto __error_return__;
        }
        err = send_client_settings();
        if (err != 0)
        {
            LOG_ERR("__cli_settings failed %s %d %s", info.ip, info.port, nghttp2_strerror(err));
            goto __error_return__;
        }
        co_return m_fd;
    }
    int client::send_client_settings()
    {
        nghttp2_settings_entry iv[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
            {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 65535},
        };

        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv, sizeof(iv) / sizeof(iv[0]));
        if (err != 0)
        {
            LOG_ERR("Failed to submit SETTINGS: %s", nghttp2_strerror(err));
            return err;
        }
        err = nghttp2_session_send(m_session);
        if (err != 0)
        {
            LOG_ERR("Failed to send SETTINGS: %s", nghttp2_strerror(err));
            return err;
        }
        return 0;
    }
}