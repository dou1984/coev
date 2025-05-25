#pragma once
#include <coev_ssl/ssl.h>
#include "session.h"

namespace coev::nghttp2
{
    class client final : public ssl::client, public session
    {
    public:
        using io_context::operator bool;
        client(SSL_CTX *);
        awaitable<int> connect(const char *url);
        int send_client_settings();
    };
}