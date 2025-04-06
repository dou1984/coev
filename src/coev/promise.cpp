#include "promise.h"
#include "co_task.h"
#include "local_resume.h"
#include "co_deliver.h"

namespace coev
{
    promise::promise() : m_tid(gtid())
    {
    }
    promise::~promise()
    {
        m_status = CORO_FINISHED;
        // LOG_CORE("promise::~promise %p %p coro:%p\n", m_task, m_caller.address(), std::coroutine_handle<promise>::from_promise(*this).address());
        if (m_task)
        {
            assert(m_caller.address() == nullptr);
            auto _task = m_task;
            m_task = nullptr;
            *_task >> this;
        }
        else if (m_caller)
        {
            assert(m_caller.address());
            local<co_list>::instance().emplace_back(m_caller, m_tid);
            m_caller = nullptr;
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
        // LOG_CORE("promise::initial_suspend %p %p %p %s\n", this, m_task, m_caller.address(), _ready ? "ready" : "not ready");
        return {
            .m_ready = _ready,
        };
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