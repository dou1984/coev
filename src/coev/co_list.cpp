#include "co_list.h"
#include "log.h"

namespace coev
{
    bool co_list::resume_all()
    {
        bool ok = false;
        while (!__list::empty())
        {
            auto caller = __list::front().m_caller;
            __list::pop_front();
            caller.resume();
            ok = true;
        }
        return ok;
    }
}