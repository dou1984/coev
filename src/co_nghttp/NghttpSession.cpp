#include <coev/coev.h>
#include "NghttpSession.h"

namespace coev::nghttp2
{

    nghttp2_session_callbacks *NghttpSession::m_callbacks = nullptr;

    struct __init_this
    {
        int _M = 0;
        __init_this()
        {
            _M = NghttpSession::__init();
        }
        ~__init_this()
        {
            NghttpSession::__finally();
        }
    } g_init_this;
    int NghttpSession::__init()
    {
        int err = nghttp2_session_callbacks_new(&m_callbacks);
        if (err != 0)
        {
            LOG_ERR("init nghttp2 failed");
            return INVALID;
        }
        nghttp2_session_callbacks_set_send_callback(m_callbacks, NghttpSession::__send_callback);
        nghttp2_session_callbacks_set_recv_callback(m_callbacks, NghttpSession::__recv_callback);
        nghttp2_session_callbacks_set_error_callback(m_callbacks, NghttpSession::__error_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(m_callbacks, NghttpSession::__on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_frame_send_callback(m_callbacks, NghttpSession::__on_frame_send_callback);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(m_callbacks, NghttpSession::__on_data_chunk_recv_callback);
        nghttp2_session_callbacks_set_on_frame_not_send_callback(m_callbacks, NghttpSession::__on_frame_not_send_callback);
        nghttp2_session_callbacks_set_on_begin_frame_callback(m_callbacks, NghttpSession::__on_begin_frame_callback);
        nghttp2_session_callbacks_set_on_stream_close_callback(m_callbacks, NghttpSession::__on_stream_close_callback);
        nghttp2_session_callbacks_set_on_begin_headers_callback(m_callbacks, NghttpSession::__on_begin_headers_callback);
        nghttp2_session_callbacks_set_on_header_callback(m_callbacks, NghttpSession::__on_header_callback);

        return 0;
    }
    int NghttpSession::__finally()
    {
        if (m_callbacks)
        {
            nghttp2_session_callbacks_del(m_callbacks);
            m_callbacks = nullptr;
        }
        return 0;
    }

    ssize_t NghttpSession::__data_source_read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
    {
        const char *data = (const char *)source->ptr;
        size_t datalen = strlen(data);
        if (length > datalen)
        {
            length = datalen;
        }
        memcpy(buf, data, length);
        if (length == datalen)
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
        }

        return length;
    }
    ssize_t NghttpSession::__send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        auto *_this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("send %ld bytes", length);
        return length;
    }
    ssize_t NghttpSession::__recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        _this->m_read_waiter.resume();
        LOG_CORE("recv %ld bytes", length);
        return length;
    }

    ssize_t NghttpSession::__read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        _this->m_read_waiter.resume();
        LOG_CORE("read %ld bytes", length);
        return length;
    }
    int NghttpSession::__error_callback(nghttp2_session *session, const char *msg, size_t len, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("error %s", msg);
        return 0;
    }
    int NghttpSession::__on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("recv frame %p", frame);
        return 0;
    }
    int NghttpSession::__on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("send frame %p", frame);
        return 0;
    }
    int NghttpSession::__on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame_hd *hd, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("begin frame %p", hd);
        return 0;
    }
    int NghttpSession::__on_frame_not_send_callback(nghttp2_session *session, const nghttp2_frame *frame, int lib_error_code, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("not send frame %p", frame);
        return 0;
    }

    int NghttpSession::__on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("begin header %p", frame);
        return 0;
    }
    int NghttpSession::__on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("header %.*s: %.*s", (int)namelen, name, (int)valuelen, value);
        return 0;
    }
    int NghttpSession::__on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("recv data %.*s", (int)len, data);
        return 0;
    }
    int NghttpSession::__on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        return 0;
    }

    NghttpSession::NghttpSession(int fd, SSL_CTX *ctx) : ssl_context(fd, ctx)
    {
    }

    NghttpSession::~NghttpSession()
    {
        if (m_session != nullptr)
        {
            nghttp2_session_del(m_session);
        }
    }

    awaitable<int> NghttpSession::send_body(nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        nghttp2_data_provider data_prd = {
            .source = {.ptr = (void *)body},
            .read_callback = &NghttpSession::__data_source_read_callback};
        auto stream_id = nghttp2_submit_request(m_session, NULL, nva, head_size, &data_prd, NULL);

        co_return stream_id;
    }
    awaitable<int> NghttpSession::recv_body(char *body, int length)
    {
        int err = co_await ssl_context::recv(body, length);
        if (err < 0)
        {

        __error_return__:
            ::close(m_fd);
            m_fd = INVALID;
            co_return 0;
        }
        err = nghttp2_session_mem_recv(m_session, (const uint8_t *)body, length);
        if (err < 0)
        {
            goto __error_return__;
        }
        co_return 0;
    }

}