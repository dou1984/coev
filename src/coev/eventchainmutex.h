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
        bool resume()
        {
            MUTEX::lock();
            auto c = static_cast<event *>(chain::pop_front());
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