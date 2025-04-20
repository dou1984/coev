#pragma once
#include "NghttpSession.h"

namespace coev::nghttp2
{
    class NghttpConnect final : public ssl_connect, public NghttpSession
    {
    public:
        NghttpConnect(SSL_CTX *);
        awaitable<int> connect(const char *url);
        int send_client_settings();
   
    };
}