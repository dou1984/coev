#include <coev/coev.h>
#include "NghttpRequest.h"

#define MAKE_NV(NAME, VALUE)  \
    {                         \
        (uint8_t *)NAME,      \
        (uint8_t *)VALUE,     \
        sizeof(NAME) - 1,     \
        sizeof(VALUE) - 1,    \
        NGHTTP2_NV_FLAG_NONE, \
    }

#define MAKE_NV_CS(NAME, VALUE) \
    {                           \
        (uint8_t *)NAME,        \
        (uint8_t *)VALUE,       \
        sizeof(NAME) - 1,       \
        strlen(VALUE),          \
        NGHTTP2_NV_FLAG_NONE,   \
    }
namespace coev::nghttp2
{
    static ssl_manager g_cli_mgr(ssl_manager::TLS_CLIENT);

    nghttp2_session_callbacks *NghttpRequest::m_callbacks = nullptr;
    int NghttpRequest::__init()
    {
        int err = nghttp2_session_callbacks_new(&m_callbacks);
        if (err != 0)
        {
            LOG_ERR("init nghttp2 failed");
            return INVALID;
        }
        nghttp2_session_callbacks_set_send_callback(m_callbacks, NghttpRequest::__send_callback);
        nghttp2_session_callbacks_set_recv_callback(m_callbacks, NghttpRequest::__recv_callback);
        nghttp2_session_callbacks_set_error_callback(m_callbacks, NghttpRequest::__error_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(m_callbacks, NghttpRequest::__on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_frame_send_callback(m_callbacks, NghttpRequest::__on_frame_send_callback);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(m_callbacks, NghttpRequest::__on_data_chunk_recv_callback);
        nghttp2_session_callbacks_set_on_frame_not_send_callback(m_callbacks, NghttpRequest::__on_frame_not_send_callback);
        nghttp2_session_callbacks_set_on_begin_frame_callback(m_callbacks, NghttpRequest::__on_begin_frame_callback);
        nghttp2_session_callbacks_set_on_stream_close_callback(m_callbacks, NghttpRequest::__on_stream_close_callback);
        nghttp2_session_callbacks_set_on_begin_headers_callback(m_callbacks, NghttpRequest::__on_begin_headers_callback);
        nghttp2_session_callbacks_set_on_header_callback(m_callbacks, NghttpRequest::__on_header_callback);

        return 0;
    }
    int NghttpRequest::__finally()
    {
        if (m_callbacks)
        {
            nghttp2_session_callbacks_del(m_callbacks);
            m_callbacks = nullptr;
        }
        return 0;
    }
    ssize_t NghttpRequest::__send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        if (flags & NGHTTP2_DATA_FLAG_EOF)
        {
            return 0;
        }
        if (flags & NGHTTP2_DATA_FLAG_NO_COPY)
        {
            return length;
        }
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("send %ld bytes", length);
        _this->m_write_waiter.resume();
        return 0;
    }

    ssize_t NghttpRequest::__recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        _this->m_read_waiter.resume();
        LOG_CORE("recv %ld bytes", length);
        return length;
    }

    ssize_t NghttpRequest::__read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        _this->m_read_waiter.resume();
        LOG_CORE("read %ld bytes", length);
        return length;
    }
    int NghttpRequest::__error_callback(nghttp2_session *session, const char *msg, size_t len, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("error %s", msg);
        return 0;
    }
    int NghttpRequest::__on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("recv frame %p", frame);
        return 0;
    }
    int NghttpRequest::__on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("send frame %p", frame);
        return 0;
    }
    int NghttpRequest::__on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame_hd *hd, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("begin frame %p", hd);
        return 0;
    }
    int NghttpRequest::__on_frame_not_send_callback(nghttp2_session *session, const nghttp2_frame *frame, int lib_error_code, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("not send frame %p", frame);
        return 0;
    }

    int NghttpRequest::__on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("begin header %p", frame);
        return 0;
    }
    int NghttpRequest::__on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("header %.*s: %.*s", (int)namelen, name, (int)valuelen, value);
        return 0;
    }
    int NghttpRequest::__on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        LOG_CORE("recv data %.*s", (int)len, data);
        return 0;
    }
    int NghttpRequest::__on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data)
    {
        auto _this = static_cast<NghttpRequest *>(user_data);
        return 0;
    }

    NghttpRequest::NghttpRequest(int fd, SSL_CTX *ctx) : ssl_connect(ctx)
    {
    }
    NghttpRequest::NghttpRequest() : ssl_connect(g_cli_mgr.get())
    {
        static int __init_nghttp2 = __init();
    }
    awaitable<int> NghttpRequest::connect(const char *url)
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
            LOG_ERR("ssl connect failed %s %d", info.ip, info.port);
            goto __error_return__;
        }
        err = nghttp2_session_client_new(&m_session, m_callbacks, this);
        if (err < 0)
        {
            LOG_ERR("nghttp2_session_client_new failed %s %d", info.ip, info.port);
            goto __error_return__;
        }
        co_return m_fd;
    }
    NghttpRequest::~NghttpRequest()
    {
        if (m_session != nullptr)
        {
            nghttp2_session_del(m_session);
        }
    }
    int NghttpRequest::__connect(const char *ip, int port)
    {
        sockaddr_in addr;
        fillAddr(addr, ip, port);
        return ::connect(m_fd, (sockaddr *)&addr, sizeof(addr));
    }
    awaitable<int> NghttpRequest::send_body(const char *body, int length)
    {
        int err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, NULL, 0);
        if (err < 0)
        {
            ::close(m_fd);
            m_fd = INVALID;
            co_return m_fd;
        }

        const nghttp2_nv nva[] = {MAKE_NV(":method", "GET"),
                                  MAKE_NV(":scheme", "https"),
                                  MAKE_NV("accept", "*/*"),
                                  MAKE_NV("user-agent", "nghttp2/" NGHTTP2_VERSION)};
        nghttp2_data_provider data_prd = {
            .source = {.ptr = this},
            .read_callback = __read_callback,
        };

        err = nghttp2_submit_response(m_session, m_fd, nva, sizeof(nva) / sizeof(nva[0]), &data_prd);
        if (err < 0)
        {
            co_return INVALID;
        }
        co_await m_write_waiter.suspend();
        co_return 0;
    }
    awaitable<int> NghttpRequest::recv_body(char *body, int length)
    {
        co_await m_read_waiter.suspend();
        co_return 0;
    }

}