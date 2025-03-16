#include "promise.h"
#include "co_task.h"

namespace coev
{
    promise::~promise()
    {
        LOG_CORE("promise::~promise %p %p\n", m_task, m_caller.address());
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

    std::suspend_never promise::initial_suspend()
    {
        LOG_CORE("promise::initial_suspend %p\n", this);
        return {};
    }
    std::suspend_never promise::final_suspend() noexcept
    {
        LOG_CORE("promise::final_suspend %p %p\n", m_task, m_caller.address());
        return {};
    }
    std::suspend_never promise_void::return_void()
    {
        LOG_CORE("promise_void::return_void\n");
        return {};
    }

}