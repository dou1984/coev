#pragma once
#include "coev_ssl/context.h"

namespace coev::sasl
{
    class SASLCli : coev::ssl::context
    {
    public:
        SASLCli() = default;
    };
}

namespace coev::pool::sasl
{
    using SASL = client_pool<sasl::SASLCli>;
}