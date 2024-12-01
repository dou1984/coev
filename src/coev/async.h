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

    namespace thread_safe
    {
        struct async : chain
        {
            std::mutex m_mutex;
            awaitable<void> suspend(const std::function<bool()> &suppend, std::function<void()> &&call);
            bool resume(const std::function<void()> &call);
        };
    }
}