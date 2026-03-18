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
        __cache(const char *_data, int length)
        {
            m_data = (char *)malloc(length);
            if (m_data == nullptr)
            {
                LOG_ERR("malloc failed");
                return;
            }
            m_length = length;
            memcpy(m_data, _data, length);
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
            return NGHTTP2_ERR_INVALID_ARGUMENT;
        }
        if (!_this->__is_processing())
        {
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        auto r = _this->__ssl_write((const char *)data, static_cast<int>(length));
        if (r == INVALID)
        {
            return NGHTTP2_ERR_CALLBACK_FAILURE;
        }
        else if (r == 0)
        {
            return r;
        }
        return r;
    }
    ssize_t session::__recv_callback(nghttp2_session *sess, uint8_t *buf, size_t length, int flags, void *user_data)
    {
        return static_cast<ssize_t>(length);
    }

    int session::__error_callback(nghttp2_session *sess, const char *msg, size_t len, void *user_data)
    {
        return 0;
    }
    int session::__on_frame_recv_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
        auto stream_id = frame->hd.stream_id;
        switch (frame->hd.type)
        {
        case NGHTTP2_DATA:
            if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
            {
                _this->__processing(stream_id);
            }
            break;
        case NGHTTP2_HEADERS:
            if (_this->__is_client())
            {
                if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
                {
                    _this->__processing(stream_id);
                }
            }
            else
            {
                if (frame->hd.flags & NGHTTP2_FLAG_END_STREAM)
                {
                    _this->__processing(stream_id);
                }
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
            if (!(frame->hd.flags & NGHTTP2_FLAG_ACK))
            {
                _this->__resume_process();
            }
            break;
        default:
            break;
        }
        return 0;
    }
    int session::__on_frame_send_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data)
    {
        return 0;
    }
    int session::__on_begin_frame_callback(nghttp2_session *sess, const nghttp2_frame_hd *hd, void *user_data)
    {
        return 0;
    }
    int session::__on_frame_not_send_callback(nghttp2_session *sess, const nghttp2_frame *frame, int lib_error_code, void *user_data)
    {
        return 0;
    }

    int session::__on_begin_headers_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data)
    {
        auto _this = static_cast<session *>(user_data);
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
        if (_this->__is_client())
        {
        }
        else
        {
            _this->remove_request(stream_id);
        }
        return 0;
    }
    session::session(SSL_CTX *ctx) : coev::ssl::context(ctx)
    {
        // 判断是否是客户端
        assert(m_type | IO_CLI);
    }
    session::session(int fd, SSL_CTX *ctx) : coev::ssl::context(fd, ctx)
    {
        auto err = nghttp2_session_server_new(&m_session, m_callbacks, this);
        if (err != 0)
        {
            __clearup();
        }
    }
    session::~session()
    {
        __clearup();
        auto _session = std::exchange(m_session, nullptr);
        while (m_waiting_write.resume())
        {
        }
        for (auto &[_, waiter] : m_w_trigger)
        {
            while (waiter.resume())
            {
            }
        }
        if (_session)
        {
            nghttp2_session_del(_session);
        }
    }

    int session::__submit_request(nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        nghttp2_data_provider data_provider{
            .source = {.ptr = new __cache(body, length)},
            .read_callback = session::__read_callback};
        auto stream_id = nghttp2_submit_request(m_session, NULL, nva, head_size, length > 0 ? &data_provider : nullptr, this);
        if (stream_id < 0)
        {
            return INVALID;
        }
        auto err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        if (err == 0)
        {
            __resume_process();
        }
        return stream_id;
    }

    int session::__submit_response(int stream_id, nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        nghttp2_data_provider data_provider{
            .source = {.ptr = new __cache(body, length)},
            .read_callback = session::__read_callback};
        auto err = nghttp2_submit_response(m_session, stream_id, nva, head_size, &data_provider);
        if (err < 0)
        {
            return INVALID;
        }
        err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        if (err == 0)
        {
            __resume_process();
        }
        return 0;
    }
    int session::__push_promise(int stream_id, nghttp2_nv *nva, int head_size)
    {
        auto r = nghttp2_submit_push_promise(m_session, NGHTTP2_FLAG_NONE, stream_id, nva, head_size, this);
        if (r < 0)
        {
            return INVALID;
        }
        r = __send();
        if (r < 0)
        {
            return INVALID;
        }
        if (r == 0)
        {
            __resume_process();
        }
        return r;
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
        assert(m_session == nullptr);
        auto err = co_await ssl::context::connect(ip, port);
        if (err == INVALID)
        {
        __error__:
            __close();
            co_return m_fd;
        }
        assert(m_ssl);
        auto _this = static_cast<session *>(this);
        err = nghttp2_session_client_new(&m_session, m_callbacks, _this);
        if (err != 0)
        {
            goto __error__;
        }
        err = send_client_settings();
        if (err != 0)
        {
            goto __error__;
        }
        m_task << __processing_read();
        m_task << __processing_write();
        co_return m_fd;
    }

    awaitable<void> session::__processing_write()
    {
        while (__is_processing())
        {
            if (nghttp2_session_want_write(m_session))
            {
                auto r = __send();
                if (r > 0)
                {
                }
                else if (r == 0)
                {
                    if (m_want_read)
                    {
                    }
                    else if (m_want_write)
                    {
                        co_await __w_waiter();
                    }
                }
                else
                {
                    __clearup();
                    break;
                }
            }
            else
            {
                co_await m_waiting_write.suspend();
            }
        }
    }
    awaitable<void> session::__processing_read()
    {
        std::string recv_buffer;
        recv_buffer.resize(64 * 1024);
        int recv_offset = 0;
        while (__is_processing())
        {
            if (recv_offset >= static_cast<int>(recv_buffer.size()))
            {
                recv_buffer.resize(recv_buffer.size() * 2);
            }
            auto r = __ssl_recv(recv_buffer.data() + recv_offset, recv_buffer.size() - recv_offset);
            if (r > 0)
            {
                recv_offset += r;
                int consumed = 0;
                while (consumed < recv_offset)
                {
                    r = nghttp2_session_mem_recv(m_session, (uint8_t *)(recv_buffer.data() + consumed), recv_offset - consumed);
                    if (r > 0)
                    {
                        consumed += r;
                    }
                    else if (r == 0)
                    {
                        break;
                    }
                    else if (r < 0)
                    {
                        co_return;
                    }
                }
                if (consumed > 0 && consumed < recv_offset)
                {
                    memmove(recv_buffer.data(), recv_buffer.data() + consumed, recv_offset - consumed);
                    recv_offset -= consumed;
                }
                else if (consumed == recv_offset)
                {
                    recv_offset = 0;
                }
            }
            else if (r == 0)
            {
                if (m_want_terminal)
                {
                    m_want_read = false;
                    m_want_write = false;
                    __clearup();
                    break;
                }
                else if (m_want_read)
                {
                    co_await m_r_waiter.suspend();
                    m_want_read = false;
                }
                else if (m_want_write)
                {
                    m_waiting_write.resume_next_loop();
                }
            }
            else if (r == INVALID)
            {
                m_want_read = false;
                m_want_write = false;
                __clearup();
                break;
            }
        }
    }
    int session::__processing(int stream_id)
    {
        if (stream_id == 0)
        {
            return 0;
        }
        if (!__is_client())
        {
            if (auto it = m_requests.find(stream_id); it != m_requests.end())
            {
                auto request = std::move(it->second);
                m_requests.erase(it);
                if (auto it_f = m_routers.find(request.path()); it_f != m_routers.end())
                {
                    m_task << [/* g++12 bug 使用 lambda 捕获参数时可能会崩溃 */](auto &_this, auto &f, auto &&_request) -> awaitable<void>
                    {
                        return f(_this, _request);
                    }(*this, it_f->second, std::move(request));
                }
            }
            if (auto it = m_w_trigger.find(stream_id); it != m_w_trigger.end())
            {
                it->second.resume();
            }
        }
        else
        {
            if (auto it = m_w_trigger.find(stream_id); it != m_w_trigger.end())
            {
                it->second.resume();
            }
        }
        return 0;
    }
    int session::__resume_process()
    {
        if (m_want_write)
        {
            m_waiting_write.resume();
        }
        if (m_session && nghttp2_session_want_write(m_session))
        {
            m_waiting_write.resume();
        }
        return 0;
    }
    bool session::__is_processing() const
    {
        return __ssl_valid() && m_session;
    }
    awaitable<int> session::query(header &h, const char *body, int length, response &res)
    {
        if (!__is_processing())
        {
            co_return INVALID;
        }
        auto stream_id = __submit_request(h, h.size(), body, length);
        if (stream_id < 0)
        {
            co_return INVALID;
        }
        co_return co_await __wait_for_stream_end(stream_id, res);
    }
    int session::reply(int stream_id, header &h, const char *body, int length)
    {
        if (!__is_processing())
        {
            return INVALID;
        }
        auto err = __submit_response(stream_id, h, h.size(), body, length);
        if (err < 0)
        {
            return INVALID;
        }
        err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        return 0;
    }
    int session::reply_error(int32_t stream_id, int error_code)
    {
        auto err = nghttp2_submit_rst_stream(m_session, NGHTTP2_FLAG_NONE, stream_id, error_code);
        if (err < 0)
        {
            return INVALID;
        }
        return err;
    }
    awaitable<int> session::__wait_for_stream_end(int stream_id, response &res)
    {
        defer(m_w_trigger.erase(stream_id););
        co_await m_w_trigger[stream_id].suspend();
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
        nghttp2_settings_entry iv[] = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv, sizeof(iv) / sizeof(iv[0]));
        if (err != 0)
        {
            LOG_ERR("Failed to submit SETTINGS: %s fd %d", nghttp2_strerror(err), m_fd);
            __clearup();
            return INVALID;
        }
        err = __send();
        if (err == INVALID)
        {
            LOG_ERR("send message failed %d %d %s fd %d", errno, err, strerror(errno), m_fd);
            __clearup();
            return INVALID;
        }
        return err;
    }
    int session::set_routers(const routers &routers)
    {
        m_routers = routers;
        return 0;
    }
    int session::send_client_settings()
    {
        nghttp2_settings_entry settings[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
            {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 65535},
            {NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE, 65535},
        };
        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, settings, 3);
        if (err != 0)
        {
            return INVALID;
        }
        err = __send();
        if (err < 0)
        {
            return INVALID;
        }
        return 0;
    }
    awaitable<int> session::do_handshake()
    {
        auto err = co_await ssl::context::do_handshake();
        if (err == INVALID)
        {
            __clearup();
            co_return INVALID;
        }
        co_return 0;
    }
    int session::__send()
    {
        if (!__is_processing())
        {
            return INVALID;
        }
        auto err = nghttp2_session_send(m_session);
        if (err != 0)
        {
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