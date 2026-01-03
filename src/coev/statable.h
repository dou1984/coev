/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include "promise.h"

namespace coev
{
    template <typename T>
    class statable
    {
    public:
        struct promise_type
        {
            std::coroutine_handle<> m_caller = nullptr;
            T value;
            promise_type() = default;

            ~promise_type() = default;

            void unhandled_exception()
            {
                throw std::current_exception();
            }

            std::suspend_always initial_suspend()
            {
                return {};
            }
            std::suspend_always final_suspend() noexcept
            {
                return {};
            }
            std::suspend_always yield_value(T &&_value)
            {
                value = std::move(_value);
                return {};
            }
            std::suspend_always yield_value(const T &_value)
            {
                value = _value;
                return {};
            }

            std::suspend_never return_void()
            {
                return {};
            }
            statable<T> get_return_object()
            {
                return {std::coroutine_handle<promise_type>::from_promise(*this)};
            }
        };
        statable() = delete;

        statable(std::coroutine_handle<promise_type> h)
        {
            m_callee = h;
        }
        operator bool()
        {
            if (m_callee)
            {
                m_callee.resume();
                return !m_callee.done();
            }
            return false;
        }
        int operator co_await() = delete;
        T &operator()()
        {
            return m_callee.promise().value;
        }
        bool done()
        {
            return m_callee && m_callee.address() && m_callee.done();
        }

        void await_suspend(std::coroutine_handle<> caller)
        {
            m_callee.promise().m_caller = caller;
        }

        auto await_resume() = delete;

    private:
        std::coroutine_handle<promise_type> m_callee = nullptr;
    };
}