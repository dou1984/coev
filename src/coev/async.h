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
        bool resume();
        bool resume_next_loop();
        int resume_all();
    };

    namespace guard
    {
        class async : queue
        {
        public:
            awaitable<void> suspend(const std::function<bool()> &, const std::function<void()> &);
            bool resume(const std::function<void()> &);
            bool deliver();
            std::mutex &lock() { return m_mutex; }

        private:
            co_event *__ev(const std::function<void()> &_set);
            std::mutex m_mutex;
        };
    }
}
