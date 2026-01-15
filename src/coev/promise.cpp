/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "promise.h"
#include "co_task.h"
#include "co_deliver.h"
#include "x.h"

namespace coev
{
    promise::promise()
    {
        LOG_CORE("promise this:%p tid:%ld", this, m_tid);
    }
    promise::~promise()
    {
        struct release
        {
            promise *m_this;
            release(promise *_this) : m_this(_this) {}
            void operator()(co_task *_task)
            {
                LOG_CORE("~promise this:%p task:%p", m_this, _task);
                assert(_task != nullptr);
                _task->unload(m_this);
            }
            void operator()(std::coroutine_handle<> _caller)
            {
                LOG_CORE("~promise this:%p caller:%p", m_this, _caller.address());
                assert(!_caller.done());
                _caller.resume();
            }
            void operator()(nullptr_t)
            {
                LOG_CORE("~promise this:%p nullptr_t", m_this);
            }
        } _(this);

        m_status = CORO_FINISHED;
        auto _that = X(m_that);
        std::visit(_, _that);
    }
    void promise::unhandled_exception()
    {
        throw std::current_exception();
    }

    suspend_bool promise::initial_suspend()
    {
        struct ready
        {
            promise *m_this;
            ready(promise *_this) : m_this(_this) {}
            bool operator()(co_task *_task)
            {
                return _task != nullptr;
            }
            bool operator()(std::coroutine_handle<> _caller)
            {
                return _caller.address() != nullptr;
            }
            bool operator()(nullptr_t) { return false; }

        } _(this);
        auto _ready = std::visit(_, m_that);
        m_status = _ready ? CORO_RUNNING : CORO_SUSPEND;
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