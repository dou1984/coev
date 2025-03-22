#include "promise.h"
#include "co_task.h"

namespace coev
{
    promise::~promise()
    {

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
        m_status = CORO_FINISHED;
    }
    void promise::unhandled_exception()
    {
        throw std::current_exception();
    }

    std::suspend_never promise::initial_suspend()
    {

        bool _ready = m_caller || m_task;
        LOG_CORE("promise::initial_suspend %p %s\n", this, _ready ? "ready" : "not ready");
        return {};
    }
    std::suspend_never promise::final_suspend() noexcept
    {
        bool _ready = m_caller || m_task;
        LOG_CORE("promise::final_suspend %p %p %s\n", this, m_task, _ready ? "ready" : "not ready");
        return {};
    }
    std::suspend_never promise_void::return_void()
    {
        LOG_CORE("promise_void::return_void\n");
        return {};
    }

}