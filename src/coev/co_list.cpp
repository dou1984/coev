#include "co_list.h"
#include "log.h"

namespace coev
{
    bool co_list::resume_all()
    {
        bool ok = false;
        while (!__list::empty())
        {
            // LOG_INFO("resume_all size %ld\n", __list::size());
            auto _co = __list::front();
            __list::pop_front();
            _co.m_caller.resume();
            ok = true;
        }
        return ok;
    }
}