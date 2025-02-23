#include "ssl_context.h"


namespace coev
{
    ssl_context::ssl_context(int fd) : io_context(fd)
    {
    }
}