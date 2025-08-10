#include "promise.h"
#include "co_task.h"
#include "co_deliver.h"
#include "x.h"

namespace coev
{
    promise::~promise()
    {
        m_status = CORO_FINISHED;
        if (m_task)
        {
            assert(m_caller.address() == nullptr);
            X(m_task)->unload(this);
        }
        else if (m_caller)
        {
            assert(!m_caller.done());
            X(m_caller).resume();
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