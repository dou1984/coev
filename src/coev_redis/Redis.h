#pragma once
#include <coev/coev.h>
#include "RedisCli.h"

namespace coev::pool
{
    struct _Redis : RedisCli
    {
        template <class T>
        _Redis(T &conf) : RedisCli(conf->host, conf->port, conf->auth)
        {
        }
    };

    using Redis = coev::client_pool<_Redis>;
}