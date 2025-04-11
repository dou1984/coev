#pragma once
#include <list>
#include <coroutine>
#include <cstdint>

namespace coev
{
    namespace details
    {
        struct __co_info
        {
            std::coroutine_handle<> m_caller = nullptr;
            uint64_t m_tid = 0;
            __co_info(std::coroutine_handle<> _caller, uint64_t _tid) : m_caller(_caller), m_tid(_tid) {}
        };
    }
    struct co_list : std::list<details::__co_info>
    {
        using __list = std::list<details::__co_info>;
        bool resume_all();
    };
}