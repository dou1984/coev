/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <climits>
#include <sys/ioctl.h>
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
        using lmp = local<memory_pool<char, 256 - 20, 1024 - 20, 4096 - 20>>;
        int m_length = 0;
        int m_offset = 0;
        char *m_data = 0;
        __cache(const char *_data, int length)
        {
            m_data = lmp::instance().create(length);
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
                lmp::instance().release(m_data);
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
        auto r = _this->__ssl_write((const char *)data, (int)length);
        if (r == INVALID)
        {
            return NGHTTP2_ERR_CALLBACK_FAILURE;
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
            LOG_CORE("session::__on_frame_recv_callback: %d", frame->hd.type)
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
    session::session(SSL_CTX *ctx) : base(ctx)
    {
        // 判断是否是客户端
        assert(m_type | IO_CLI);
    }
    session::session(int fd, SSL_CTX *ctx) : base(fd, ctx)
    {
        auto err = nghttp2_session_server_new(&m_session, m_callbacks, this);
        if (err != 0)
        {
            __clearup();
        }
    }
    session::~session()
    {
        auto _session = std::exchange(m_session, nullptr);
        while (m_write_waiter.resume())
        {
        }
        for (auto &[_, waiter] : m_end_trigger)
        {
            while (waiter.resume(INVALID))
            {
            }
        }
        if (_session)
        {
            nghttp2_session_del(_session);
        }
        __clearup();
    }

    awaitable<int> session::__submit_request(nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        auto cache = new __cache(body, length);
        nghttp2_data_provider data_provider{
            .source = {.ptr = cache},
            .read_callback = session::__read_callback};
        auto stream_id = nghttp2_submit_request(m_session, NULL, nva, head_size, length > 0 ? &data_provider : nullptr, this);
        if (stream_id < 0)
        {
            delete cache;
            co_return INVALID;
        }

        auto err = co_await __send();
        if (err < 0)
        {
            co_return INVALID;
        }
        co_return stream_id;
    }

    awaitable<int> session::__submit_response(int stream_id, nghttp2_nv *nva, int head_size, const char *body, int length)
    {
        auto cache = new __cache(body, length);
        nghttp2_data_provider data_provider{
            .source = {.ptr = cache},
            .read_callback = session::__read_callback};
        auto err = nghttp2_submit_response(m_session, stream_id, nva, head_size, &data_provider);
        if (err < 0)
        {
            delete cache;
            co_return INVALID;
        }
        err = co_await __send();
        if (err < 0)
        {
            co_return INVALID;
        }
        co_return 0;
    }
    // awaitable<int> session::__push_promise(int stream_id, nghttp2_nv *nva, int head_size)
    // {
    //     auto r = nghttp2_submit_push_promise(m_session, NGHTTP2_FLAG_NONE, stream_id, nva, head_size, this);
    //     if (r < 0)
    //     {
    //         co_return INVALID;
    //     }
    //     r = co_await __send();
    //     if (r < 0)
    //     {
    //         return INVALID;
    //     }
    //     return r;
    // }

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
        err = nghttp2_session_client_new(&m_session, m_callbacks, this);
        if (err != 0)
        {
            goto __error__;
        }
        err = co_await send_client_settings();
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
        LOG_CORE("processing write session %p", this);
        finally(LOG_CORE("end processing write session %p", this));
        int loop_count = 0;
        int zero_count = 0;
        while (__is_processing())
        {
            loop_count++;
            if (loop_count % 1000 == 0)
            {
                LOG_ERR("[DBG] session %p processing_write loop %d, zero_count=%d, m_want_read=%d, m_want_write=%d", this, loop_count, zero_count, m_want_read, m_want_write);
            }
            if (nghttp2_session_want_write(m_session))
            {
                LOG_CORE("[DBG] session %p before __send, m_want_read=%d, m_want_write=%d", this, m_want_read, m_want_write);
                auto r = co_await __send();
                LOG_CORE("[DBG] session %p after __send, r=%d, m_want_read=%d, m_want_write=%d", this, r, m_want_read, m_want_write);
                if (r > 0)
                {
                    LOG_CORE("session %p send %d bytes", this, r);
                }
                else if (r == 0)
                {
                    zero_count++;
                    if (zero_count % 100 == 0)
                    {
                        LOG_ERR("[DBG] session %p send returned 0, zero_count=%d, m_want_read=%d, m_want_write=%d", this, zero_count, m_want_read, m_want_write);
                    }

                    r = co_await __w_waiter();
                    if (r == INVALID)
                    {
                        co_return;
                    }
                }
                else
                {
                    LOG_CORE("send error %d break", r);
                    __clearup();
                    break;
                }
            }
            else
            {

                LOG_CORE("fd %d write waiter 2", m_fd);
                finally(LOG_CORE("fd %d write waiter 2 end", m_fd));
                auto r = co_await m_write_waiter.suspend();
                if (r == INVALID)
                {
                    co_return;
                }
            }
        }
    }
    awaitable<void> session::__processing_read()
    {
        LOG_CORE("processing read session %p", this);
        finally(LOG_CORE("finished processing read session %p", this));
        std::string recv_buffer;
        recv_buffer.resize(4 * 1024);
        int recv_offset = 0;
        while (__is_processing())
        {
            if (recv_offset >= static_cast<int>(recv_buffer.size()))
            {
                recv_buffer.resize(recv_buffer.size() * 2);
            }

            int r = __ssl_read(recv_buffer.data() + recv_offset, recv_buffer.size() - recv_offset);
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
                        LOG_CORE("session::parse: nghttp2_session_mem_recv: break");
                        break;
                    }
                    else if (r < 0)
                    {
                        LOG_CORE("ssl_read error %d", r);
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
                    LOG_CORE("session::__processing_read() break");
                    break;
                }
                else if (m_want_read)
                {
                    LOG_CORE("session %p want read", this);
                    finally(LOG_CORE("end session %p want read", this));
                    r = co_await __r_waiter();
                    if (r == INVALID)
                    {
                        __clearup();
                        co_return;
                    }
                }
                else if (m_want_write)
                {
                    LOG_CORE("session %p want write", this);
                    finally(LOG_CORE("end session %p want write", this));
                    r = co_await __w_waiter();
                    if (r == INVALID)
                    {
                        __clearup();
                        co_return;
                    }
                }
            }
            else if (r == INVALID)
            {
                m_want_read = false;
                m_want_write = false;
                __clearup();
                LOG_CORE("session::__processing_ssl_write() break");
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
                m_task << __router(std::move(it->second));
                m_requests.erase(it);
            }
            if (auto it = m_end_trigger.find(stream_id); it != m_end_trigger.end())
            {
                it->second.resume_next_loop();
            }
        }
        else
        {
            if (auto it = m_end_trigger.find(stream_id); it != m_end_trigger.end())
            {
                it->second.resume_next_loop();
            }
        }
        return 0;
    }
    awaitable<void> session::__router(request &&_request)
    {
        if (auto it = m_routers->find(_request.path()); it != m_routers->end())
        {
            return it->second(*this, _request);
        }
        auto dummy = [](auto &, auto &) -> awaitable<void>
        {
            co_return;
        };
        return dummy(*this, _request);
    }

    int session::__resume_process()
    {
        assert(m_session);
        if (nghttp2_session_want_write(m_session))
        {
            m_write_waiter.resume_next_loop();
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
        auto stream_id = co_await __submit_request(h, h.size(), body, length);
        if (stream_id < 0)
        {
            co_return INVALID;
        }
        co_return co_await __wait_for_stream_end(stream_id, res);
    }
    awaitable<int> session::reply(int stream_id, header &h, const char *body, int length)
    {
        if (!__is_processing())
        {
            co_return INVALID;
        }
        auto err = co_await __submit_response(stream_id, h, h.size(), body, length);
        if (err < 0)
        {
            co_return INVALID;
        }
        err = co_await __send();
        if (err < 0)
        {
            co_return INVALID;
        }
        co_return 0;
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
        finally(m_end_trigger.erase(stream_id));
        co_await m_end_trigger[stream_id].suspend();
        if (auto it = m_responses.find(stream_id); it != m_responses.end())
        {
            res = std::move(it->second);
            m_responses.erase(it);
            co_return 0;
        }
        co_return INVALID;
    }
    int session::set_routers(std::shared_ptr<routers> &routers)
    {
        m_routers = routers;
        return 0;
    }

    awaitable<int> session::send_server_settings()
    {
        nghttp2_settings_entry iv[] = {{NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100}};
        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, iv, sizeof(iv) / sizeof(iv[0]));
        if (err != 0)
        {
            LOG_ERR("Failed to submit SETTINGS: %s fd %d", nghttp2_strerror(err), m_fd);
            __clearup();
            co_return INVALID;
        }
        err = co_await __send();
        if (err == INVALID)
        {
            LOG_ERR("send message failed %d %d %s fd %d", errno, err, strerror(errno), m_fd);
            __clearup();
            co_return INVALID;
        }
        co_return err;
    }
    awaitable<int> session::send_client_settings()
    {
        nghttp2_settings_entry settings[] = {
            {NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 100},
            {NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE, 65535},
            {NGHTTP2_SETTINGS_MAX_HEADER_LIST_SIZE, 65535},
        };
        auto err = nghttp2_submit_settings(m_session, NGHTTP2_FLAG_NONE, settings, 3);
        if (err != 0)
        {
            co_return INVALID;
        }
        err = co_await __send();
        if (err < 0)
        {
            co_return INVALID;
        }
        co_return 0;
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
    awaitable<int> session::__send()
    {
        if (!__is_processing())
        {
            co_return INVALID;
        }
        LOG_CORE("[DBG] session %p __send ENTER, m_want_read=%d, m_want_write=%d", this, m_want_read, m_want_write);
        while (nghttp2_session_want_write(m_session))
        {
            LOG_CORE("[DBG] session %p __send nghttp2_session_send, m_want_read=%d, m_want_write=%d", this, m_want_read, m_want_write);
            auto r = nghttp2_session_send(m_session);
            LOG_CORE("[DBG] session %p __send nghttp2_session_send returned %d, m_want_read=%d, m_want_write=%d", this, r, m_want_read, m_want_write);
            if (r > 0)
            {
                LOG_CORE("[DBG] session %p __send returned %d (success)", this, r);
                co_return r;
            }
            else if (r == 0)
            {
                LOG_CORE("[DBG] session %p __send r==0, m_want_read=%d, m_want_write=%d", this, m_want_read, m_want_write);
                if (m_want_read)
                {
                    LOG_CORE("[DBG] session %p __send resuming m_r_waiter", this);
                    if (!m_r_waiter.resume())
                    {
                        LOG_CORE("session::__send m_r_waiter resume failed");
                    }
                }
                if (m_want_write)
                {
                    LOG_CORE("[DBG] session %p __send waiting for writable", this);
                    r = co_await __w_waiter();
                    LOG_CORE("[DBG] session %p __send after __w_waiter, r=%d", this, r);
                    if (r == INVALID)
                    {
                        co_return r;
                    }
                }
            }
            else
            {
                LOG_CORE("[DBG] session %p __send returned %d (error)", this, r);
                co_return r;
            }
        }
        LOG_CORE("[DBG] session %p __send returned 0 (complete)", this);
        co_return 0;
    }
    int session::__sync_send()
    {
        if (!__is_processing())
        {
            return INVALID;
        }
        if (nghttp2_session_want_write(m_session))
        {
            return nghttp2_session_send(m_session);
        }
        return 0;
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

    int session::__ssl_write(const char *buffer, int buffer_size)
    {
        if (buffer_size <= 0)
        {
            return INVALID;
        }

        int send_offset = 0;
        if (!__is_ssl())
        {
            assert(m_ssl == nullptr);
            while (send_offset < buffer_size)
            {
                int r = ::send(m_fd, buffer + send_offset, buffer_size - send_offset, MSG_NOSIGNAL);
                if (r > 0)
                {
                    send_offset += r;
                    continue;
                }
                else if (r == INVALID && (errno == EAGAIN || errno == EWOULDBLOCK))
                {
                    m_want_write = true;
                    return send_offset > 0 ? send_offset : 0;
                }
                else
                {
                    return INVALID;
                }
            }
            return send_offset;
        }

        if (!__ssl_valid())
        {
            return INVALID;
        }

        while (send_offset < buffer_size)
        {
            m_want_write = false;
            m_want_read = false;
            int r = SSL_write(m_ssl, buffer + send_offset, buffer_size - send_offset);
            LOG_CORE("[DBG] session %p __ssl_write SSL_write returned %d, send_offset=%d/%d", this, r, send_offset, buffer_size);

            if (r > 0)
            {
                send_offset += r;
                continue;
            }

            r = SSL_get_error(m_ssl, r);
            LOG_CORE("[DBG] session %p __ssl_write SSL_get_error returned %d", this, r);
            if (r == SSL_ERROR_WANT_WRITE)
            {
                m_want_write = true;
                LOG_CORE("[DBG] session %p __ssl_write WANT_WRITE, returning %d", this, send_offset > 0 ? send_offset : 0);
                return send_offset > 0 ? send_offset : 0;
            }
            else if (r == SSL_ERROR_WANT_READ)
            {
                m_want_read = true;
                LOG_CORE("[DBG] session %p __ssl_write WANT_READ, returning %d", this, send_offset > 0 ? send_offset : 0);
                return send_offset > 0 ? send_offset : 0;
            }
            else
            {
                LOG_CORE("[DBG] session %p __ssl_write error %d, returning INVALID", this, r);
                errno = -r;
                return INVALID;
            }
        }

        return send_offset;
    }
    int session::__ssl_read(char *buffer, int size)
    {
        if (m_ssl == nullptr)
        {
            while (true)
            {
                m_want_read = false;
                int r = ::recv(m_fd, buffer, size, 0);
                if (r > 0)
                {
                    return r;
                }
                else if (r == INVALID && (errno == EAGAIN || errno == EWOULDBLOCK))
                {
                    m_want_read = true;
                    return 0;
                }
                else if (r == 0)
                {
                    m_want_terminal = true;
                    return 0;
                }
                else
                {
                    errno = ECONNRESET;
                    return INVALID;
                }
            }
        }

        if (!__ssl_valid())
        {
            return INVALID;
        }

        while (true)
        {
            m_want_read = false;
            m_want_write = false;
            int r = SSL_read(m_ssl, buffer, size);
            LOG_CORE("[DBG] session %p __ssl_read SSL_read returned %d", this, r);
            if (r > 0)
            {
                return r;
            }

            r = SSL_get_error(m_ssl, r);
            LOG_CORE("[DBG] session %p __ssl_read SSL_get_error returned %d", this, r);
            if (r == SSL_ERROR_WANT_READ)
            {
                m_want_read = true;
                LOG_CORE("[DBG] __ssl_read WANT_READ, returning 0 %p", this);
                return 0;
            }
            else if (r == SSL_ERROR_WANT_WRITE)
            {
                m_want_write = true;
                LOG_CORE("[DBG] __ssl_read WANT_WRITE, returning 0 %p", this);
                return 0;
            }
            else if (r == SSL_ERROR_ZERO_RETURN)
            {
                m_want_terminal = true;
                return 0;
            }
            else if (r == SSL_ERROR_SSL)
            {
                errno = ECONNRESET;
                return INVALID;
            }
            else
            {
                errno = ECONNRESET;
                return INVALID;
            }
        }
    }
}