#include "is_destroying.h"

namespace coev
{
    is_destroying::operator bool() const
    {
        return m_status;
    }
    void is_destroying::lock()
    {
        m_status = 1;
    }
    void is_destroying::unlock()
    {
        m_status = 0;
    }
}