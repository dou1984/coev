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
        };
    }
    struct co_list : std::list<details::__co_info>
    {
        using __list = std::list<details::__co_info>;
        bool resume_all();
    };
}