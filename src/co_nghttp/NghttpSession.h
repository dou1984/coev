#pragma once
#include <coev/coev.h>
#include <nghttp2/nghttp2.h>
#include <openssl/ssl.h>
#include "Ngheader.h"

namespace coev::nghttp2
{
    class NghttpSession : virtual protected ssl_context
    {
    public:
        using ssl_context::do_handshake;
        NghttpSession(int, SSL_CTX *);
        NghttpSession(const NghttpSession &) = delete;
        NghttpSession &operator=(const NghttpSession &) = delete;
        ~NghttpSession();

        awaitable<int> send_body(nghttp2_nv *, int head_size, const char *body, int length);
        awaitable<int> recv_body(char *body, int length);

    private:
        static ssize_t __data_source_read_callback(
            nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
            uint32_t *data_flags, nghttp2_data_source *source, void *user_data);
        static ssize_t __send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);
        static ssize_t __recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data);
        static ssize_t __read_callback(
            nghttp2_session *session, int32_t stream_id, uint8_t *buf, size_t length,
            uint32_t *data_flags, nghttp2_data_source *source, void *user_data);
        static int __error_callback(nghttp2_session *session, const char *msg, size_t len, void *user_data);
        static int __on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static int __on_frame_send_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static int __on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame_hd *hd, void *user_data);
        static int __on_frame_not_send_callback(nghttp2_session *session, const nghttp2_frame *frame, int lib_error_code, void *user_data);
        static int __on_stream_close_callback(nghttp2_session *session, int32_t stream_id, uint32_t error_code, void *user_data);
        static int __on_begin_headers_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static int __on_header_callback(nghttp2_session *session, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data);
        static int __on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);

        friend class __init_this;
        static int __init();
        static int __finally();

    protected:
        NghttpSession() = default;
        nghttp2_session *m_session = nullptr;
        static nghttp2_session_callbacks *m_callbacks;
    };
}