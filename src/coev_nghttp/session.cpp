/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <climits>
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
        int m_length = 0;
        int m_offset = 0;
        char *m_data = 0;
        __cache(const char *_data) : m_data(strdup(_data))
        {
            m_length = strlen(m_data);
        }
        ~__cache()
        {
            if (m_data)
            {
                free(m_data);
            }
        }
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
            auto _callbacks = std::exchange(m_callbacks, nullptr);
            nghttp2_session_callbacks_del(_callbacks);
        }
        return 0;
    }

    ssize_t session::__read_callback(nghttp2_session *sess, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        auto cache = (__cache *)source->ptr;
        if (cache->m_offset == cache->m_length)
        {
            *data_flags |= NGHTTP2_DATA_FLAG_EOF;
            delete cache;
            LOG_CORE("stream_id %d EOF %d", stream_id, *data_flags);
            return 0;
        }
        if (length > (cache->m_length - cache->m_offset))
        {
            length = cache->m_length - cache->m_offset;
        }
        memcpy(buf, cache->m_data + cache->m_offset, length);
        cache->m_offset += length;
        return length;
    }
    ssize_t session::__send_callback(nghttp2_session *sess, const uint8_t *data, size_t length, int flags, void *user_data)
    {
        auto *_this = static_cast<session *>(user_data);
        if (length > INT_MAX)
        {
            LOG_CORE("send failed: length %ld exceeds INT_MAX", length);
            return INVALID;
        }
        auto r = _this->__ssl_write((const char *)data, static_cast<int>(length));
        if (r == INVALID)
        {
            LOG_CORE("send failed %d %s, tid: %ld", errno, strerror(errno), gtid());
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
        if (frame->hd.type != NGHTTP2_HEADERS)
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
    session::session(SSL_CTX *ctx) : coev::ssl::context(ctx)
    {
        assert(m_type | IO_CLI);
    }
    session::session(int fd, SSL_CTX *ctx) : coev::ssl::context(fd, ctx)
    {
        auto err = nghttp2_session_server_new(&m_session, m_callbacks, this);
        if (err != 0)
        {
            LOG_ERR("nghttp2_session_client_new failed %s", nghttp2_strerror(err));
            __clearup();
        }
    }
    session::~session()
    {
        if (m_session != nullptr)
        {
            auto _session = std::exchange(m_session, nullptr);
            LOG_CORE("del session %p", _session);
            nghttp2_session_del(_session);
        }
    }

    int session::__submit_request(nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        nghttp2_data_provider data_provider{
            .source = {.ptr = new __cache(body)},
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
        nghttp2_data_provider data_provider{
            .source = {.ptr = new __cache(body)},
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

    awaitable<int> session::connect(const char *url) noexcept
    {
        if (m_fd == INVALID)
        {
            co_return m_fd;
        }
        addrInfo info;
        int err = info.fromUrl(url);
        if (err == INVALID)
        {
            __clearup();
            co_return m_fd;
        }
        co_return co_await connect(info.ip, info.port);
    }
    awaitable<int> session::connect(const char *ip, int port) noexcept
    {
        auto err = co_await ssl::context::connect(ip, port);
        if (err == INVALID)
        {
            LOG_CORE("ssl connect failed %s %d %s", ip, port, nghttp2_strerror(err));
        __error_return__:
            __close();
            co_return m_fd;
        }
        assert(m_ssl);

        auto _this = static_cast<session *>(this);
        err = nghttp2_session_client_new(&m_session, m_callbacks, _this);
        if (err != 0)
        {
            LOG_CORE("nghttp2_session_client_new failed %s %d %s", ip, port, nghttp2_strerror(err));
            goto __error_return__;
        }
        err = send_client_settings();
        if (err != 0)
        {
            LOG_CORE("__cli_settings failed %s %d %s", ip, port, nghttp2_strerror(err));
            goto __error_return__;
        }

        processing();
        co_return m_fd;
    }

    awaitable<int> session::__processing()
    {
        std::string recv_buffer;
        recv_buffer.resize(4096);
        int recv_offset = 0;
        defer(__clearup());
        while (__ssl_valid())
        {
            auto r = co_await ssl::context::recv(recv_buffer.data() + recv_offset, recv_buffer.size() - recv_offset);
            if (r < 0)
            {
                LOG_CORE("recv failed %d %d %s", r, errno, strerror(errno));
                co_return INVALID;
            }
            if (!__ssl_valid())
            {
                co_return INVALID;
            }
            recv_offset += r;
            r = nghttp2_session_mem_recv(m_session, (uint8_t *)recv_buffer.data(), recv_offset);
            if (r < 0)
            {
                LOG_CORE("recv failed %d %d %s", errno, r, nghttp2_strerror(r));
                co_return INVALID;
            }
            if (r > 0)
            {
                recv_offset -= r;
                memmove(recv_buffer.data(), recv_buffer.data() + r, recv_offset);
            }
            if (nghttp2_session_want_write(m_session))
            {
                auto err = __send();
                if (err < 0)
                {
                    LOG_CORE("send failed %d %s", err, nghttp2_strerror(err));
                    co_return INVALID;
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
                m_requests.erase(it);
                if (auto it_r = routers.find(req.path()); it_r != routers.end())
                {
                    LOG_CORE("stream_id %d received END_STREAM flag", req.id());
                    m_task << it_r->second(*this, req);
                }
            }
        }
        co_return 0;
    }
    awaitable<int> session::query(header &h, const char *body, int length, response &res)
    {
        auto stream_id = __submit_request(h, h.size(), body, length);
        if (stream_id < 0)
        {
            co_return INVALID;
        }
        co_return co_await __wait_for_stream_end(stream_id, res);
    }
    awaitable<int> session::reply(int stream_id, header &h, const char *body, int length)
    {
        auto err = __submit_response(stream_id, h, h.size(), body, length);
        if (err < 0)
        {
            co_return INVALID;
        }
        response res;
        co_return co_await __wait_for_stream_end(stream_id, res);
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
    awaitable<int> session::__wait_for_stream_end(int stream_id, response &res)
    {
        co_await m_w_trigger[stream_id].suspend();
        m_w_trigger.erase(stream_id);
        if (auto it = m_responses.find(stream_id); it != m_responses.end())
        {
            res = std::move(it->second);
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
            __clearup();
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
    int session::send_client_settings()
    {
        nghttp2_settings_entry iv[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
            {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 65535 * 16},
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
    awaitable<int> session::do_handshake()
    {
        auto err = co_await ssl::context::do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake failed error %d %s", errno, strerror(errno));
            __clearup();
            co_return INVALID;
        }
        co_return 0;
    }
    int session::__send()
    {
        if (!__ssl_valid())
        {
            return INVALID;
        }
        auto err = nghttp2_session_send(m_session);
        if (err != 0)
        {
            LOG_ERR("send failed %d %d %s", errno, err, nghttp2_strerror(err));
            return INVALID;
        }
        return err;
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