#pragma once
#include <coev/coev.h>
#include <nghttp2/nghttp2.h>
#include "ng_session.h"

namespace coev::nghttp2
{

    class ng_server : public coev::tcp::server
    {
    public:
        ng_server(const char *url);
        int route(const std::string &path, const ng_session::router &);
        awaitable<void> dispatch(SSL_CTX *_mgr);

    protected:
        awaitable<int> __dispatch(int fd, SSL_CTX *_mgr);

    private:
        ng_session::routers m_routers;
        co_task m_tasks;
    };
}