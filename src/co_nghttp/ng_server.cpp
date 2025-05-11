#include <coev/coev.h>
#include "ng_server.h"

namespace coev::nghttp2
{

    ng_server::ng_server(const char *url)
    {
        addrInfo info;
        int err = info.fromUrl(url);
        if (err == INVALID)
        {
            LOG_ERR("invalid url %s", url);
            return;
        }
        err = tcp::server::start(info.ip, info.port);
        if (err == INVALID)
        {
            LOG_ERR("start server failed\n");
        }
    }
    int ng_server::route(const std::string &path, const ng_session::router &_route)
    {
        m_routers.emplace(path, _route);
        return 0;
    }
    awaitable<void> ng_server::dispatch(SSL_CTX *_manager)
    {
        while (valid())
        {
            addrInfo info;
            int fd = co_await accept(info);

            LOG_CORE("recv fd %d from %s:%d\n", fd, info.ip, info.port);
            m_tasks << __dispatch(fd, _manager);
        }
    }
    awaitable<int> ng_server::__dispatch(int fd, SSL_CTX *_manager)
    {
        LOG_CORE("client start %d\n", fd);
        ng_session ctx(fd, _manager);
        auto err = co_await ctx.do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake error %d %s\n", errno, strerror(errno));
            co_return INVALID;
        }
        ctx.send_server_settings();
        co_await wait_for_all(ctx.processing(), ctx.on_stream_end(m_routers));
        co_return 0;
    }

}