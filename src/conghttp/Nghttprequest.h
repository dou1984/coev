#pragma once
#include <coev/coev.h>
#include <nghttp2/nghttp2.h>
#include <openssl/ssl.h>
#include <coev/socket.h>
#include <cosys/cosys.h>

namespace coev::http2
{
    class Nghttprequest : protected client
    {
    public:
        Nghttprequest();
        ~Nghttprequest();

        awaitable<int> connect(const char *url);
        awaitable<int> send_body(const char *body, int length);
        awaitable<int> recv_body(char *body, int length);

    protected:
        static ssize_t __send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);
        static ssize_t __recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data);

    private:
        nghttp2_session *m_session = nullptr;
        nghttp2_session_callbacks *m_callbacks = nullptr;
        int m_fd = INVALID;
        coev::awaitable m_send_waiter;
        coev::awaitable m_recv_waiter;
    };
}