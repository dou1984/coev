#pragma once
#include <unordered_map>
#include <nghttp2/nghttp2.h>
#include <openssl/ssl.h>
#include <coev/coev.h>
#include <co_ssl/co_ssl.h>
#include "NgRequest.h"

namespace coev::nghttp2
{
    class NgSession : virtual protected coev::ssl::ssl_context
    {
    public:
        awaitable<int> do_handshake();

        NgSession(int, SSL_CTX *);
        NgSession(const NgSession &) = delete;
        NgSession &operator=(const NgSession &) = delete;
        ~NgSession();

        awaitable<int> processing();

        awaitable<int> submit_request(nghttp2_nv *, int head_size);
        awaitable<int> submit_body(nghttp2_nv *, int head_size, const char *body, int length);
        awaitable<int> submit_response(int stream_id, nghttp2_nv *, int head_size, const char *body, int length);

        int send_server_settings();

    protected:
        NgSession() = default;
        nghttp2_session *m_session = nullptr;
        static nghttp2_session_callbacks *m_callbacks;

        std::unordered_map<uint32_t, NgRequest> m_requests;
        std::unordered_map<uint32_t, coev::async> m_streams;

    private:
        int __send_body();
        static ssize_t __send_callback2(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);
        static ssize_t __recv_callback2(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data);
        static ssize_t __read_callback2(nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data);
        static int __error_callback2(nghttp2_session *session, int lib_error_code, const char *msg, size_t len, void *user_data);
        static int __on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static int __on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static int __on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame_hd *hd, void *user_data);
        static int __on_frame_not_send_callback(nghttp2_session *session, const nghttp2_frame *frame, int lib_error_code, void *user_data);
        static int __on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data);
        static int __on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static int __on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data);
        static int __on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);

        friend struct __init_this;
        static int __init();
        static int __finally();
    };
}