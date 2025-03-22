#include <coev/cosys/cosys.h>
#include "NghttpServer.h"

namespace coev::nghttp2
{
    NghttpServer::NghttpServer(const char *url)
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

}