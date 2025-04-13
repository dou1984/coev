#pragma once
#include <coroutine>
#include <stdint.h>
#include <list>
#include <set>

namespace coev
{
    class co_caller
    {
        using handle = std::coroutine_handle<>;
        std::list<handle> m_callers;

    public:
        co_caller() = default;
        ~co_caller();
        void push(handle _caller);
        bool resume();
        bool resume_all();
    };

}