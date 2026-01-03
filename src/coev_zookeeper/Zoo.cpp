/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <atomic>
#include <chrono>
#include "Zoo.h"

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

    const clientid_t &clientid_t::operator=(const clientid_t &o)
    {
        client_id = o.client_id;
        strncpy(passwd, o.passwd, sizeof(passwd));
        return *this;
    }
    void clientid_t::clear()
    {
        client_id = 0;
        memset(passwd, 0, sizeof(passwd));
    }
}