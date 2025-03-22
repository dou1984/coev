#pragma once
#include "awaken.h"
#include "coev.h"

namespace coev
{
    class co_pipe final : protected awaken
    {
        uint64_t m_tid = 0;

    public:
        co_pipe();
        ~co_pipe();

        static void resume(async &listener);
    };
}
