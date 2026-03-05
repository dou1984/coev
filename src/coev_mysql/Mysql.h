#pragma once

#include <coev/coev.h>
#include "MysqlCli.h"

namespace coev::pool
{
    struct _Mysql : MysqlCli
    {
        template <class T>
        _Mysql(T &conf) : MysqlCli({conf->host, conf->port, conf->username, conf->password, conf->options["db"], conf->options["charset"]})
        {
        }
        operator bool() const
        {
            return fd() != INVALID && [](const MysqlCli *cli)
            {
                return (MYSQL *)(*cli) != nullptr;
            }(this);
        }
    };

    using Mysql = client_pool<_Mysql>;
}