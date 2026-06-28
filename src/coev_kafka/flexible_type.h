#pragma once
#include <stdint.h>
#include <cassert>

namespace coev::kafka
{
    struct FlexibleType
    {
        uint16_t m_flexible = 0;
        bool __is_fixed();
        bool __is_flexible();
        void __push_flexible();
        void __pop_flexible();
    };
}