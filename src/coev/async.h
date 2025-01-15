#pragma once
#include <mutex>
#include <functional>
#include "chain.h"
#include "event.h"
#include "awaitable.h"

namespace coev
{
    struct async : chain
    {
        event suspend();
        bool resume();
    };

    namespace guard
    {
        struct async : chain
        {
            std::mutex m_mutex;
            awaitable<void> suspend(const std::function<bool()> &, const std::function<void()> &);
            bool resume(const std::function<void()> &);
        };
    }
}