#include <coev/coev.h>
#include "NghttpSession.h"

namespace coev::nghttp2
{
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
    };
    static __init_this g_init_this;
    struct __cache
    {
        const char *m_data = nullptr;
        int m_length = 0;
    };

    nghttp2_session_callbacks *NghttpSession::m_callbacks = nullptr;
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

    ssize_t NghttpSession::__read_callback(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
    {
        if (source == nullptr)
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            LOG_CORE("data_source_read_callback source is null\n");
            return 0;
        }
        auto _this = static_cast<NghttpSession *>(user_data);
        auto cache = (__cache *)source->ptr;
        LOG_CORE("data_source_read_callback %ld bytes length %d from %s\n", length, cache->m_length, cache->m_data);
        if (cache->m_length == 0)
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            LOG_CORE("stream_id %d EOF %d\n", stream_id, *data_flags);
            return 0;
        }
        if (length > cache->m_length)
        {
            length = cache->m_length;
        }
        memcpy(buf, cache->m_data, length);
        cache->m_data += length;
        cache->m_length -= length;
        return length;
    }
    ssize_t NghttpSession::__send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        auto *_this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("send %ld bytes\n", length);
        auto r = _this->__ssl_write((const char *)data, length);
        if (r == INVALID)
        {
            LOG_CORE("send failed %d\n", errno);
        }
        return r;
    }
    ssize_t NghttpSession::__recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data)
    {
        LOG_CORE("recv %ld bytes stream_id flags %d", length, flags);
        auto _this = static_cast<NghttpSession *>(user_data);
        _this->m_read_waiter.resume();
        return length;
    }
    int NghttpSession::__error_callback(nghttp2_session *session, const char *msg, size_t len, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("error %s\n", msg);
        return 0;
    }
    int NghttpSession::__on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("recv stream_id %d type %d flags %d\n", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        auto stream_id = frame->hd.stream_id;
        if (frame->hd.type == NGHTTP2_SETTINGS)
        {
        }
        else if (frame->hd.type == NGHTTP2_GOAWAY)
        {
            _this->m_requests.erase(stream_id);
        }
        else if (frame->hd.type == NGHTTP2_DATA)
        {
        }
        else if (frame->hd.type == NGHTTP2_HEADERS)
        {
        }
        else if (frame->hd.type == NGHTTP2_RST_STREAM)
        {
        }
        return 0;
    }
    int NghttpSession::__on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("send stream_id %d type %d flags %d\n", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        return 0;
    }
    int NghttpSession::__on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame_hd *hd, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("begin frame %p stream_id %d\n", hd, hd->stream_id);
        _this->m_requests[hd->stream_id] = NghttpRequest();
        return 0;
    }
    int NghttpSession::__on_frame_not_send_callback(nghttp2_session *session, const nghttp2_frame *frame, int lib_error_code, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("not send frame %p\n", frame);
        return 0;
    }

    int NghttpSession::__on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("begin header %d type %d flags %d\n", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        return 0;
    }
    int NghttpSession::__on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("header stream_id %d %.*s: %.*s\n", frame->hd.stream_id, (int)namelen, name, (int)valuelen, value);
        _this->m_requests[frame->hd.stream_id].push_back((const char *)name, namelen, (const char *)value, valuelen);
        return 0;
    }
    int NghttpSession::__on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("recv data chunk %.*s flags:%d\n", (int)len, data, flags);
        if (flags & NGHTTP2_FLAG_END_STREAM)
        {
            LOG_CORE("stream_id %d received END_STREAM flag\n", stream_id);
            // 设置标志位，通知主循环停止接收
            // NghttpSession *self = static_cast<NghttpSession *>(user_data);
            // self->set_stream_complete(stream_id);
        }
        return 0;
    }
    int NghttpSession::__on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data)
    {
        auto _this = static_cast<NghttpSession *>(user_data);
        LOG_CORE("__on_stream_close_callback stream_id %d error %d\n", stream_id, error_code);
        return 0;
    }

    NghttpSession::NghttpSession(int fd, SSL_CTX *ctx) : io_context(fd), ssl_context(fd, ctx)
    {
        auto err = nghttp2_session_server_new(&m_session, m_callbacks, this);
        if (err != 0)
        {
            LOG_ERR("nghttp2_session_client_new failed %s\n", nghttp2_strerror(err));
            __close();
        }
    }

    NghttpSession::~NghttpSession()
    {
        if (m_session != nullptr)
        {
            nghttp2_session_del(m_session);
        }
    }

    awaitable<int> NghttpSession::submit_request(nghttp2_nv *nva, int head_size)
    {

        auto stream_id = nghttp2_submit_request(m_session, NULL, nva, head_size, NULL, this);
        auto err = __send_body();
        if (err < 0)
        {
            co_return INVALID;
        }
        co_await m_streams[stream_id].suspend();

        
    }
    awaitable<int> NghttpSession::process_loop()
    {
        while (__valid())
        {
            char body[1024];
            auto r = co_await ssl_context::recv(body, sizeof(body));
            if (r < 0)
            {
                LOG_CORE("recv failed %d %d %s\n", errno, r, strerror(errno));
            __error_return__:
                __close();
                co_return INVALID;
            }
            LOG_CORE("sess %p recv %d bytes %s\n", m_session, r, body);
            r = nghttp2_session_mem_recv(m_session, (uint8_t *)body, r);
            if (r < 0)
            {
                LOG_CORE("recv failed %d %d %s\n", errno, r, nghttp2_strerror(r));
                goto __error_return__;
            }
            LOG_CORE("mem recv %d bytes\n", r);
        }
        co_return 0;
    }

    int NghttpSession::send_server_settings()
    {
        nghttp2_settings_entry iv[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv, sizeof(iv) / sizeof(iv[0]));
        if (err != 0)
        {
            LOG_ERR("Failed to submit SETTINGS: %s\n", nghttp2_strerror(err));
        __error_return__:
            __close();
            return INVALID;
        }
        err = __send_body();
        if (err < 0)
        {
            LOG_ERR("send message failed %d %d %s\n", errno, err, strerror(errno));
            goto __error_return__;
        }
        return err;
    }
    awaitable<int> NghttpSession::do_handshake()
    {
        auto err = co_await ssl_context::do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake failed error %d %s\n", errno, strerror(errno));
            __close();
            co_return INVALID;
        }
        co_return 0;
    }
    int NghttpSession::__send_body()
    {
        auto r = nghttp2_session_send(m_session);
        if (r < 0)
        {
            LOG_ERR("send failed %d %d %s\n", errno, r, nghttp2_strerror(r));
            __close();
            return INVALID;
        }
        return r;
    }

}