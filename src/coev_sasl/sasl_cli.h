#pragma once
#include "coev_ssl/client.h"

namespace coev ::sasl
{
    class SASLCli : ssl::sclient
    {
    public:
        SASLCli() = default;
    };
}

namespace coev::pool::sasl
{
    using SASL = client_pool<sasl::SASLCli>;
}