/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <cassert>

#include "is_destroying.h"

namespace coev
{
    is_destroying::operator bool() const
    {
        assert(m_status >= 0);
        return m_status > 0;
    }
    void is_destroying::lock()
    {
        m_status += 1;
    }
    void is_destroying::unlock()
    {
        m_status -= 1;
    }
}