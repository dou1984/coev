#pragma once
#include <mutex>
#include <functional>
#include "queue.h"
#include "co_event.h"
#include "awaitable.h"

namespace coev
{
    struct async : queue
    {
        co_event suspend();
        bool resume(bool immediate = false);        
        void resume_all();
    };

    namespace guard
    {
        struct async : queue
        {
            std::mutex m_mutex;
            awaitable<void> suspend(const std::function<bool()> &, const std::function<void()> &);
            bool resume(const std::function<void()> &);
        };
    }
}