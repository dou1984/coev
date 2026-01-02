#pragma once
#include <string>

namespace tls
{
    struct Config
    {
        std::string ServerName;
    };
    enum ConnectionState
    {
    };
    struct Conn
    {
        auto ConnectionState()
        {
            return 0;
        }
    };
}