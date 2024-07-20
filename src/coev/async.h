#pragma once
#include <mutex>
#include "chain.h"

namespace coev
{
    struct async : chain
    {
    };

    namespace ts
    {
        struct async : chain, std::mutex
        {
        };
    }

}