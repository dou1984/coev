#pragma once
#include "co_deliver.h"
#include "awaitable.h"

namespace coev
{
    class co_pipe final : protected co_deliver
    {
        uint64_t m_tid = 0;

    public:
        co_pipe();
        ~co_pipe();

        awaitable<void> operator<<();
        awaitable<void> operator>>();

        static void resume(async &listener);
    };
}
