#pragma once
#include <co_ssl/co_ssl.h>
#include "ng_session.h"

namespace coev::nghttp2
{
    class ng_client final : public coev::ssl::ssl_client, public ng_session
    {
    public:
        using io_context::operator bool;
        ng_client(SSL_CTX *);
        awaitable<int> connect(const char *url);
        int send_client_settings();
    };
}