/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <utility>
#include "promise.h"
#include "co_task.h"
#include "co_deliver.h"

namespace coev
{
    promise::promise()
    {
    }
    promise::~promise()
    {
        m_status = details::CORO_FINISHED;
        if (m_type == details::CORO_COROUTINE_HANDLE)
        {
            auto _caller = std::exchange(m_caller, nullptr);
            _caller.resume();
        }
        else if (m_type == details::CORO_TASK)
        {
            auto _task = static_cast<co_task *>(std::exchange(m_task, nullptr));
            assert(_task != nullptr);
            _task->unload(this);
        }
        else if (m_type == details::CORO_GUARD_TASK)
        {
            auto _task = static_cast<guard::co_task *>(std::exchange(m_task, nullptr));
            assert(_task != nullptr);
            _task->unload(this);
        }
        else
        {
            assert(false);
        }
    }
    void promise::unhandled_exception()
    {
        throw std::current_exception();
    }

    suspend_ready promise::initial_suspend()
    {
        auto _ready = m_type != details::CORO_NONE;
        m_status = _ready ? details::CORO_RUNNING : details::CORO_SUSPEND;
        return {.m_ready = _ready};
    }

    std::suspend_never promise::final_suspend() noexcept
    {
        return {};
    }
    std::suspend_never promise_void::return_void()
    {
        return {};
    }
}