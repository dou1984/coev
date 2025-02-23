#pragma once
#include <coev/coev.h>
#include "io_context.h"

namespace coev
{

    class ssl_context : public io_context
    {
    public:
        ssl_context(int fd);
    };
}