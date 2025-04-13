#include "async.h"
#include "local_resume.h"
#include "co_caller.h"

namespace coev
{
    bool local_resume()
    {
        bool ok = false;
        while (local<async>::instance().resume_all())
        {
            ok = true;
        }
        // while (local<co_caller>::instance().resume_all())
        // {
        //     ok = true;
        // }
        return ok;
    }
}