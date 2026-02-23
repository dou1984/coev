/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coroutine>
#include "async.h"
#include "local_resume.h"
#include "local.h"

namespace coev
{
    bool local_resume()
    {
        bool ok = false;
        while (local_async::instance().resume_all())
        {
            ok = true;
        }
        return ok;
    }
}