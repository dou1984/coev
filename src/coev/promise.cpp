#include "promise.h"
#include "co_task.h"

namespace coev
{
    promise::~promise()
    {
        m_status = CORO_FINISHED;
        LOG_CORE("promise::~promise %p %p coro:%p\n", m_task, m_caller.address(), std::coroutine_handle<promise>::from_promise(*this).address());
        if (m_task)
        {
            assert(m_caller.address() == nullptr);
            auto _task = m_task;
            m_task = nullptr;
            _task->done(this);
        }
        else if (m_caller)
        {
            assert(m_caller.address());
            auto _caller = m_caller;
            m_caller = nullptr;
            _caller.resume();
        }
    }
    void promise::unhandled_exception()
    {
        throw std::current_exception();
    }

    suspend_bool promise::initial_suspend()
    {
        auto _ready = m_caller || m_task;
        m_status = _ready ? CORO_RUNNING : CORO_SUSPEND;
        LOG_CORE("promise::initial_suspend %p %p %p %s\n", this, m_task, m_caller.address(), _ready ? "ready" : "not ready");
        return {
            .m_ready = _ready,
        };
    }

    std::suspend_never promise::final_suspend() noexcept
    {
        LOG_CORE("promise::final_suspend %p %p\n", this, m_caller.address());
        return {};
    }
    std::suspend_never promise_void::return_void()
    {
        LOG_CORE("promise_void::return_void %p %p\n", this, m_caller.address());
        return {};
    }

}