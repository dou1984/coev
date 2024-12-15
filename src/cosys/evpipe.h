#pragma once
#include "awaken.h"
#include "coev/coev.h"

namespace coev
{
    class evpipe final : protected awaken
    {
        uint64_t m_tid = 0;

    public:
        evpipe();
        ~evpipe();

        static void resume(async &listener);
    };
}
