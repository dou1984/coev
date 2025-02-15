#include <coev/coev.h>
#include "Nghttprequest.h"

namespace coev::http2
{

    static nghttp2_session_callbacks *g_callback;
    int init()
    {
        nghttp2_session_callbacks_new(&g_callbacks);

        nghttp2_session_callbacks_set_send_callback(g_callbacks, Nghttprequest::__send_callback);
        nghttp2_session_callbacks_set_recv_callback(g_callbacks, Nghttprequest::__recv_callback);
        nghttp2_session_callbacks_set_error_callback(g_callbacks, Nghttprequest::__error_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(g_callbacks, Nghttprequest::__on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(g_callbacks, Nghttprequest::__on_data_chunk_recv_callback);
        nghttp2_session_callbacks_set_on_frame_send_callback(g_callbacks, Nghttprequest::__on_frame_send_callback);
        return 0;
    }
    static int finally()
    {
        nghttp2_session_callbacks_destroy(g_callbacks);
        return 0;
    }
    static int __init_nghttp = init();
    defer(finally);

    ssize_t Nghttprequest::__send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        if (flags & NGHTTP2_DATA_FLAG_EOF)
        {
            return 0;
        }
        if (flags & NGHTTP2_DATA_FLAG_NO_COPY)
        {
            return length;
        }
        auto _this = static_cast<Nghttprequest *>(user_data);
        _this->m_write_waiter.resume();
        return 0;
    }

    ssize_t Nghttprequest::__recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
    {

        auto _this = static_cast<Nghttprequest *>(user_data);
        _this->m_read_waiter.resume();
        return length;
    }

    Nghttprequest::Nghttprequest()
    {
    }

    Nghttprequest::~Nghttprequest()
    {
        if (m_session != nullptr)
        {
            nghttp2_session_del(m_session);
        }
        if (m_callbacks != nullptr)
        {
            nghttp2_session_callbacks_del(m_callbacks);
        }
    }

    awaitable<int> connect(const char *url);
    {
        int m_fd = co_await connect(url, 443);
        if (m_fd == INVALID)
        {
            co_return INVALID;
        }
        int err = nghttp2_session_client_new(&m_session, m_callbacks, this);
        if (err < 0)
        {
            co_return INVALID;
        }
        co_return m_fd;
    }
    awaitable<int> Nghttprequest::send_body(const char *body, int length)
    {
        co_await m_send_waiter.suspend();
        co_return 0;
    }
    awaitable<int> Nghttprequest::recv_body(char *body, int length)
    {
        co_await m_recv_waiter.suspend();
        co_return 0;
    }

}