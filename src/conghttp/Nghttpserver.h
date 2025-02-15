#pragma once

#include <cosys/cosys.h>
#include <nghttp2/nghttp2.h>

namespace coev::http2
{

    class Nghttpserver : protected tcp::server
    {
    public:
        Nghttpserver(const char *host, int port);
        ~Nghttpserver();
        void stand();

    protected:
        static ssize_t __send_callback(nghttp2_session *session, const uint8_t *data, size_t length, int flags, void *user_data);
        static ssize_t __recv_callback(nghttp2_session *session, uint8_t *buf, size_t length, int flags, void *user_data);
        static ssize_t __on_frame_recv_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
        static ssize_t __on_begin_frame_callback(nghttp2_session *session, const nghttp2_frame *frame, void *user_data);
    private:
        nghttp2_session *m_session = nullptr;
        nghttp2_session_callbacks *m_callbacks = nullptr;
        nghttp2_hd_deflater *m_deflater = nullptr;
    };
}