#pragma once
#include <unordered_map>
#include <nghttp2/nghttp2.h>
#include <openssl/ssl.h>
#include <coev/coev.h>
#include <co_ssl/co_ssl.h>
#include "ng_request.h"
#include "ng_response.h"

namespace coev::nghttp2
{

    class ng_session : virtual protected coev::ssl::ssl_context
    {
    public:
        using router = std::function<awaitable<int>(ng_session &, ng_request &)>;
        using routers = std::unordered_map<std::string, router>;

        awaitable<int> do_handshake();

        ng_session(int, SSL_CTX *);
        ng_session(const ng_session &) = delete;
        ng_session &operator=(const ng_session &) = delete;
        ~ng_session();

        awaitable<int> processing();
        awaitable<int> on_stream_end(const routers &);
        awaitable<ng_response> wait_for_stream_end(int stream_id);

        int submit_request(nghttp2_nv *, int head_size, const char *body, int length);
        int submit_response(int stream_id, nghttp2_nv *, int head_size, const char *body, int length);
        int push_promise(int stream_id, nghttp2_nv *, int head_size);
        int error_reply(int32_t id, int error_code);

    public:
        int set_routers(const routers &);
        ng_request &get_request(int32_t stream_id);
        void remove_request(int32_t stream_id);
        ng_response &get_response(int32_t stream_id);
        void remove_response(int32_t stream_id);

        int send_server_settings();

    protected:
        ng_session() = default;
        nghttp2_session *m_session = nullptr;
        static nghttp2_session_callbacks *m_callbacks;
        std::unordered_map<int32_t, ng_request> m_requests;
        std::unordered_map<int32_t, ng_response> m_responses;

    protected:
        co_task m_tasks;
        async m_trigger;
        std::unordered_map<int32_t, async> m_w_trigger;

    private:
        int __send();
        int __processing(int stream_id);

        static ssize_t __send_callback(nghttp2_session *sess, const uint8_t *data, size_t length, int flags, void *user_data);
        static ssize_t __recv_callback(nghttp2_session *sess, uint8_t *buf, size_t length, int flags, void *user_data);
        static ssize_t __read_callback(nghttp2_session *sess, int32_t stream_id, uint8_t *buf, size_t length, uint32_t *data_flags, nghttp2_data_source *source, void *user_data);
        static int __error_callback(nghttp2_session *sess, const char *msg, size_t len, void *user_data);
        static int __on_frame_recv_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data);
        static int __on_frame_send_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data);
        static int __on_begin_frame_callback(nghttp2_session *sess, const nghttp2_frame_hd *hd, void *user_data);
        static int __on_frame_not_send_callback(nghttp2_session *sess, const nghttp2_frame *frame, int lib_error_code, void *user_data);
        static int __on_stream_close_callback(nghttp2_session *sess, int32_t stream_id, uint32_t error_code, void *user_data);
        static int __on_begin_headers_callback(nghttp2_session *sess, const nghttp2_frame *frame, void *user_data);
        static int __on_header_callback(nghttp2_session *sess, const nghttp2_frame *frame, const uint8_t *name, size_t namelen, const uint8_t *value, size_t valuelen, uint8_t flags, void *user_data);
        static int __on_data_chunk_recv_callback(nghttp2_session *sess, uint8_t flags, int32_t stream_id, const uint8_t *data, size_t len, void *user_data);

        friend struct __init_this;
        static int __init();
        static int __finally();
    };

}