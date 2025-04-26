#include <coev/coev.h>
#include "NgServer.h"

namespace coev::nghttp2
{

    NgServer::NgServer(const char *url)
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
    int NgServer::route(const std::string &path, const cb_route &_route)
    {
        m_route.emplace(path, _route);
        return 0;
    }
    awaitable<void> NgServer::dispatch(SSL_CTX *_manager)
    {
        while (valid())
        {
            addrInfo info;
            int fd = co_await accept(info);

            LOG_DBG("recv fd %d from %s:%d\n", fd, info.ip, info.port);
            co_start << [=]() -> awaitable<int>
            {
                LOG_DBG("client start %d\n", fd);
                NgSession ctx(fd, _manager);
                auto err = co_await ctx.do_handshake();
                if (err == INVALID)
                {
                    LOG_ERR("handshake error %d %s\n", errno, strerror(errno));
                    co_return INVALID;
                }
                ctx.send_server_settings();
                co_await ctx.processing();
                co_return 0;
            }();
        }
    }
}