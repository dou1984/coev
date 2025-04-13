#include <cassert>
#include "co_caller.h"

namespace coev
{
    co_caller::~co_caller()
    {
        resume_all();
    }
    void co_caller::push(handle _caller)
    {
        m_callers.emplace_back(_caller);
    }
    bool co_caller::resume()
    {
        if (!m_callers.empty())
        {
            auto c = m_callers.front();
            m_callers.pop_front();
            c.resume();
            return true;
        }
        return false;
    }
    bool co_caller::resume_all()
    {
        bool ok = false;
        while (resume())
        {
            ok = true;
        }
        return ok;
    }
}