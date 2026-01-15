#include <coev/coev.h>
#include "server.h"

namespace coev::nghttp2
{
    server::server(const char *url)
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
            LOG_ERR("start server failed");
        }
    }
    int server::route(const std::string &path, const session::router &_route)
    {
        m_routers.emplace(path, _route);
        return 0;
    }
    awaitable<void> server::dispatch(SSL_CTX *_manager)
    {
        while (valid())
        {
            addrInfo info;
            int fd = co_await accept(info);

            LOG_CORE("recv fd %d from %s:%d", fd, info.ip, info.port);
            m_tasks << __dispatch(fd, _manager);
        }
    }
    awaitable<int> server::__dispatch(int fd, SSL_CTX *_manager)
    {
        LOG_CORE("client start %d", fd);
        session ctx(fd, _manager);
        auto err = co_await ctx.do_handshake();
        if (err == INVALID)
        {
            LOG_ERR("handshake error %d %s", errno, strerror(errno));
            co_return INVALID;
        }
        ctx.send_server_settings();
        // co_await wait_for_all(ctx.processing(), ctx.on_stream(m_routers));
        co_start << ctx.processing();
        co_await ctx.on_stream(m_routers);
        co_return 0;
    }

}