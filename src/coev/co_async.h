/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <mutex>
#include <functional>
#include "queue.h"
#include "co_event.h"
#include "awaitable.h"

namespace coev
{

    struct co_async : queue
    {
        co_event suspend() noexcept;
        co_event suspend_util_next_loop() noexcept;
        awaitable<int64_t> suspend(const std::function<bool()> &) noexcept;
        bool resume(int64_t value = 0) noexcept;
        bool resume_next_loop() noexcept;
        int resume_all(int64_t) noexcept;
    };

    namespace guard
    {
        struct co_event_f;
        class co_async : queue
        {
        public:
            ~co_async();

            awaitable<int64_t> suspend(const std::function<bool()> &_suspend, const std::function<void()> &_getter = []() {}) noexcept;
            bool resume(const std::function<void()> &) noexcept;
            bool deliver(const std::function<void()> &) noexcept;
            bool deliver(uint64_t value = 0) noexcept;

            std::mutex &lock() noexcept { return m_mutex; }

        private:
            co_event_f *__ev(const std::function<void()> &_set);
            std::mutex m_mutex;
        };
    }
    using local_async = local<co_async>;
}
