#include <atomic>
#include <chrono>
#include "Zoo.h"
#
namespace coev
{
    int32_t get_xid()
    {
        static std::atomic<int32_t> xid = {-1};
        if (xid == -1)
        {
            xid = time(0);
        }
        return ++xid;
    }

}