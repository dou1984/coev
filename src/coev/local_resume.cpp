#include "local_resume.h"
#include "async.h"
#include "co_list.h"

namespace coev
{
    bool local_resume()
    {
        bool ok = false;
        while (local<async>::instance().resume_all())
        {
            ok = true;
        }
        return ok;
    }
}