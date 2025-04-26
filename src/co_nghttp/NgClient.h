#pragma once
#include "NgSession.h"

namespace coev::nghttp2
{
    class NgClient final : public coev::ssl::ssl_client, public NgSession
    {
    public:
        NgClient(SSL_CTX *);
        awaitable<int> connect(const char *url);
        int send_client_settings();
   
    };
}