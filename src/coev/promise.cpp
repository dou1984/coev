#include "promise.h"
#include "co_task.h"
#include "local_resume.h"
#include "co_deliver.h"

namespace coev
{
    promise::~promise()
    {
        m_status = CORO_FINISHED;
        if (m_task)
        {
            assert(m_caller.address() == nullptr);
            auto _task = m_task;
            m_task = nullptr;
            *_task >> this;
        }
        else if (m_caller)
        {
            assert(!m_caller.done());
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