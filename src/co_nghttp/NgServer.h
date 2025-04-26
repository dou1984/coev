#pragma once
#include <coev/coev.h>
#include <nghttp2/nghttp2.h>
#include "NgSession.h"

namespace coev::nghttp2
{

    class NgServer : public coev::tcp::server
    {
    public:
        using cb_route = awaitable<int> (*)(NgSession &, NgRequest &);
        NgServer(const char *url);

        int route(const std::string &path, const cb_route &);

        awaitable<void> dispatch(SSL_CTX *_mgr);

    private:
        std::unordered_map<std::string, cb_route> m_route;
    };
}