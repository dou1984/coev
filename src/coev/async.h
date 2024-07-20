#pragma once
#include <mutex>
#include "chain.h"

namespace coev
{
    struct async : chain
    {
    };

    struct async_ts : chain, std::mutex
    {
    };

}