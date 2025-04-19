#include "async.h"
#include "local_resume.h"
#include "local.h"

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