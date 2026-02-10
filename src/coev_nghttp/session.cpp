#include <coev/coev.h>
#include "session.h"

namespace coev::nghttp2
{
    struct __init_this
    {
        int _M = 0;
        __init_this()
        {
            _M = session::__init();
        }
        ~__init_this()
        {
            session::__finally();
        }
    };
    static __init_this g_init_this;
    struct __cache
    {
        const char *m_data = nullptr;
        int m_length = 0;
    };

    nghttp2_session_callbacks *session::m_callbacks = nullptr;
    int session::__init()
    {
        int err = nghttp2_session_callbacks_new(&m_callbacks);
        if (err != 0)
        {
            LOG_ERR("init nghttp2 failed");
            return INVALID;
        }
        nghttp2_session_callbacks_set_send_callback(m_callbacks, session::__send_callback);
        nghttp2_session_callbacks_set_recv_callback(m_callbacks, session::__recv_callback);
        nghttp2_session_callbacks_set_error_callback(m_callbacks, session::__error_callback);
        nghttp2_session_callbacks_set_on_frame_recv_callback(m_callbacks, session::__on_frame_recv_callback);
        nghttp2_session_callbacks_set_on_frame_send_callback(m_callbacks, session::__on_frame_send_callback);
        nghttp2_session_callbacks_set_on_data_chunk_recv_callback(m_callbacks, session::__on_data_chunk_recv_callback);
        nghttp2_session_callbacks_set_on_frame_not_send_callback(m_callbacks, session::__on_frame_not_send_callback);
        nghttp2_session_callbacks_set_on_begin_frame_callback(m_callbacks, session::__on_begin_frame_callback);
        nghttp2_session_callbacks_set_on_stream_close_callback(m_callbacks, session::__on_stream_close_callback);
        nghttp2_session_callbacks_set_on_begin_headers_callback(m_callbacks, session::__on_begin_headers_callback);
        nghttp2_session_callbacks_set_on_header_callback(m_callbacks, session::__on_header_callback);

        return 0;
    }
    int session::__finally()
    {
        if (m_callbacks)
        {
            nghttp2_session_callbacks_del(m_callbacks);
            m_callbacks = nullptr;
        }
        return 0;
    }

    ssize_t session::__read_callback(nghttp2_session *sess, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        auto cache = (__cache *)source->ptr;
        if (cache->m_length == 0)
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            LOG_CORE("stream_id %d EOF %d", stream_id, *data_flags);
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
    ssize_t session::__send_callback(nghttp2_session *sess, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        auto *_this = static_cast<session *>(user_data);
        LOG_CORE("send %ld bytes", length);
        auto r = _this->__ssl_write((const char *)data, length);
        if (r == INVALID)
        {
            LOG_CORE("send failed %d %s", errno, strerror(errno));
        }
        return r;
    }
    ssize_t session::__recv_callback(nghttp2_session *sess, uint8_t *buf, size_t length, int flags, void *user_data)
    {
        LOG_CORE("recv %ld bytes stream_id flags %d", length, flags);
        auto _this = static_cast<session *>(user_data);
        _this->m_r_waiter.resume();
        return length;
    }

    int session::__error_callback(nghttp2_session *sess, const char *msg, size_t len, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("error %s", msg);
        return 0;
    }
    int session::__on_frame_recv_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("recv stream_id %d type %d flags %d", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        auto stream_id = frame->hd.stream_id;
        switch (frame->hd.type)
        {
        case NGHTTP2_DATA:
        case NGHTTP2_HEADERS:
            if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
            {
                _this->__processing(stream_id);
            }
            break;
        case NGHTTP2_GOAWAY:
        case NGHTTP2_RST_STREAM:
            if (_this->__is_client())
            {
                _this->remove_response(stream_id);
            }
            else
            {
                _this->remove_request(stream_id);
            }
            _this->__processing(stream_id);
            break;
        case NGHTTP2_SETTINGS:
        case NGHTTP2_PUSH_PROMISE:
        default:
            break;
        }
        return 0;
    }
    int session::__on_frame_send_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("send stream_id %d type %d flags %d", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        return 0;
    }
    int session::__on_begin_frame_callback(nghttp2_session *sess, const nghttp2_frame_hd *hd, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("begin frame %p stream_id %d type %d flags %d", hd, hd->stream_id, hd->type, hd->flags);
        return 0;
    }
    int session::__on_frame_not_send_callback(nghttp2_session *sess, const nghttp2_frame *frame, int lib_error_code, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("not send frame %d type %d flags %d", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        return 0;
    }

    int session::__on_begin_headers_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("begin header %d type %d flags %d", frame->hd.stream_id, frame->hd.type, frame->hd.flags);
        if (frame->hd.type != NGHTTP2_HEADERS || frame->headers.cat != NGHTTP2_HCAT_REQUEST)
        {
            return 0;
        }
        auto stream_id = frame->hd.stream_id;
        if (_this->__is_client())
        {
            auto &res = _this->get_response(stream_id);
            res.set_stream_id(stream_id);
        }
        else
        {
            auto &req = _this->get_request(stream_id);
            req.set_stream_id(stream_id);
        }
        return 0;
    }
    int session::__on_header_callback(nghttp2_session *sess, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("header stream_id %d flags %d type %d length %.*s: %.*s", frame->hd.stream_id, frame->hd.flags, frame->hd.type, (int)namelen, name, (int)valuelen, value);
        if (frame->headers.cat != NGHTTP2_HCAT_REQUEST)
        {
            return 0;
        }
        auto stream_id = frame->hd.stream_id;
        if (_this->__is_client())
        {
            auto &res = _this->get_response(stream_id);
            res.add_header((const char *)name, namelen, (const char *)value, valuelen);
        }
        else
        {
            auto &req = _this->get_request(stream_id);
            req.add_header((const char *)name, namelen, (const char *)value, valuelen);
        }
        return 0;
    }
    int session::__on_data_chunk_recv_callback(nghttp2_session *sess, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("recv data chunk %.*s flags:%d", (int)len, data, flags);
        if (_this->__is_client())
        {
            auto &res = _this->get_response(stream_id);
            res.append((const char *)data, len);
        }
        else
        {
            auto &req = _this->get_request(stream_id);
            req.append((const char *)data, len);
        }
        return 0;
    }
    int session::__on_stream_close_callback(nghttp2_session *sess, int32_t stream_id, uint32_t error_code, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        LOG_CORE("__on_stream_close_callback stream_id %d error %d", stream_id, error_code);
        if (_this->__is_client())
        {
            // _this->remove_response(stream_id);
        }
        else
        {
            _this->remove_request(stream_id);
        }
        return 0;
    }

    session::session(int fd, SSL_CTX *ctx) : io_context(fd), ssl::context(fd, ctx)
    {
        auto err = nghttp2_session_server_new(&m_session, m_callbacks, this);
        if (err != 0)
        {
            LOG_ERR("nghttp2_session_client_new failed %s", nghttp2_strerror(err));
            __close();
        }
    }

    session::~session()
    {
        if (m_session != nullptr)
        {
            nghttp2_session_del(m_session);
        }
    }

    int session::__submit_request(nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        __cache cache = {.m_data = body, .m_length = length};
        nghttp2_data_provider data_provider = {
            .source = {.ptr = &cache},
            .read_callback = session::__read_callback};
        auto stream_id = nghttp2_submit_request(m_session, NULL, nva, head_size, &data_provider, this);
        if (stream_id < 0)
        {
            return INVALID;
        }
        auto err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        return stream_id;
    }

    int session::__submit_response(int stream_id, nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        __cache cache = {.m_data = body, .m_length = length};
        nghttp2_data_provider data_provider = {
            .source = {.ptr = &cache},
            .read_callback = session::__read_callback};
        auto err = nghttp2_submit_response(m_session, stream_id, nva, head_size, &data_provider);
        if (err < 0)
        {
            LOG_CORE("submit response failed %d %s", err, nghttp2_strerror(err));
            return INVALID;
        }
        err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        return 0;
    }
    int session::__push_promise(int stream_id, nghttp2_nv *nva, int head_size)
    {
        auto err = nghttp2_submit_push_promise(m_session, NGHTTP2_FLAG_NONE, stream_id, nva, head_size, this);
        if (err < 0)
        {
            LOG_CORE("submit response failed %d %s", err, nghttp2_strerror(err));
            return INVALID;
        }
        err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        return 0;
    }
    int session::reply(int stream_id, header &h, const char *body, int length)
    {
        return __submit_response(stream_id, h, h.size(), body, length);
    }
    int session::reply_error(int32_t stream_id, int error_code)
    {
        auto err = nghttp2_submit_rst_stream(m_session, NGHTTP2_FLAG_NONE, stream_id, error_code);
        if (err < 0)
        {
            LOG_CORE("submit rst stream failed %d %s", err, nghttp2_strerror(err));
            return INVALID;
        }
        return err;
    }

    awaitable<int> session::processing()
    {
        while (__valid())
        {
            char body[1024 * 4];
            auto r = co_await ssl::context::recv(body, sizeof(body));
            if (r < 0)
            {
                LOG_CORE("recv failed %d %d %s", r, errno, strerror(errno));
            __error_return__:
                __close();
                co_return INVALID;
            }
            LOG_CORE("sess %p recv %d %.*s", m_session, r, (int)r, body);
            r = nghttp2_session_mem_recv(m_session, (uint8_t *)body, r);
            if (r < 0)
            {
                LOG_CORE("recv failed %d %d %s", errno, r, nghttp2_strerror(r));
                goto __error_return__;
            }
            if (nghttp2_session_want_write(m_session) == 0)
            {
                auto err = __send();
                if (err < 0)
                {
                    LOG_CORE("send failed %d %s", err, nghttp2_strerror(err));
                    goto __error_return__;
                }
            }
        }
    }

    int session::__processing(int stream_id)
    {
        if (!__is_client())
        {
            m_trigger.resume();
        }
        else if (auto it = m_w_trigger.find(stream_id); it != m_w_trigger.end())
        {
            it->second.resume_next_loop();
        }
        return 0;
    }
    awaitable<int> session::on_stream(const routers &routers)
    {
        while (*this)
        {
            co_await m_trigger.suspend();
            auto stream_id = nghttp2_session_get_last_proc_stream_id(m_session);
            if (auto it = m_requests.find(stream_id); it != m_requests.end())
            {
                auto req = std::move(it->second);
                if (auto it_r = routers.find(req.path()); it_r != routers.end())
                {
                    auto err = __push_promise(stream_id, nullptr, 0);
                    if (err < 0)
                    {
                        LOG_CORE("push promise failed %d %s", err, nghttp2_strerror(err));
                        __close();
                        co_return INVALID;
                    }
                    LOG_CORE("stream_id %d received END_STREAM flag", req.id());
                    m_tasks << it_r->second(*this, req);
                }
            }
        }
        co_return 0;
    }
    awaitable<response> session::query(header &h, const char *body, int length)
    {

        auto stream_id = __submit_request(h, h.size(), body, length);
        if (stream_id < 0)
        {
            static response _;
            co_return _;
        }
        response res;
        co_await __wait_for_stream_end(stream_id, res);
        co_return std::move(res);
    }
    awaitable<int> session::__wait_for_stream_end(int stream_id, response &req)
    {
        co_await m_w_trigger[stream_id].suspend();
        m_w_trigger.erase(stream_id);

        if (auto it = m_responses.find(stream_id); it != m_responses.end())
        {
            req = std::move(it->second);
            m_responses.erase(it);
            co_return 0;
        }
        co_return INVALID;
    }
    int session::send_server_settings()
    {
        nghttp2_settings_entry iv[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};

        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv, sizeof(iv) / sizeof(iv[0]));
        if (err != 0)
        {
            LOG_ERR("Failed to submit SETTINGS: %s", nghttp2_strerror(err));
        __error_return__:
            __close();
            return INVALID;
        }
        err = __send();
        if (err < 0)
        {
            LOG_ERR("send message failed %d %d %s", errno, err, strerror(errno));
            goto __error_return__;
        }
        return err;
    }
    awaitable<int> session::do_handshake()
    {
        auto err = co_await ssl::context::do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake failed error %d %s", errno, strerror(errno));
            __close();
            co_return INVALID;
        }
        co_return 0;
    }
    int session::__send()
    {
        auto r = nghttp2_session_send(m_session);
        if (r != 0)
        {
            LOG_ERR("send failed %d %d %s", errno, r, nghttp2_strerror(r));
            __close();
            return INVALID;
        }
        return r;
    }
    request &session::get_request(int32_t stream_id)
    {
        return m_requests[stream_id];
    }
    void session::remove_request(int32_t stream_id)
    {
        m_requests.erase(stream_id);
    }
    response &session::get_response(int32_t stream_id)
    {
        return m_responses[stream_id];
    }
    void session::remove_response(int32_t stream_id)
    {
        m_responses.erase(stream_id);
    }
}