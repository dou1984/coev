#pragma once
#include <coev/coev.h>
#include <nghttp2/nghttp2.h>
#include "session.h"

namespace coev::nghttp2
{

    class server final : public tcp::server
    {
    public:
        server(const char *url);
        int route(const std::string &path, const session::router &);
        awaitable<void> dispatch(SSL_CTX *_mgr);

    protected:
        awaitable<int> __dispatch(int fd, SSL_CTX *_mgr);

    private:
        session::routers m_routers;
        co_task m_tasks;
    };
}