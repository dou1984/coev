/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023, Zhao Yun Shan
 *	All rights reserved.
 *
 */
#pragma once
#include <mutex>
#include "chain.h"
#include "object.h"
#include "event.h"
#include "awaiter.h"

namespace coev
{
    template <class TYPE, class MUTEX>
    struct eventchainmutex : chain, MUTEX, TYPE
    {
        void append(chain *c)
        {
            chain::push_back(c);
            MUTEX::unlock();
        }

        template <class SUSPEND, class CALL>
        awaiter wait_for(const SUSPEND &suppend, const CALL &call)
        {
            MUTEX::lock();
            if (suppend())
            {
                co_await event(this);
                MUTEX::lock();
            }
            call();
            MUTEX::unlock();
            co_return 0;
        }
        template <class CALL>
        bool resume(const CALL &call)
        {
            MUTEX::lock();
            auto c = static_cast<event *>(chain::pop_front());
            call();
            MUTEX::unlock();
            if (c)
            {
                c->resume();
                return true;
            }
            return false;
        }
    };
    using EVMutex = eventchainmutex<RECV, std::mutex>;
}