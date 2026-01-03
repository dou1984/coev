/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#pragma once
#include <queue>
#include "awaitable.h"

namespace coev
{
    template <class CLI>
    class client_pool
    {
        using client_list = std::list<CLI>;
        class instance
        {
            client_list &m_connections;
            
        public:
            instance(client_list &pool) : m_this_pool(pool)
            {
            }
            ~instance()
            {

            }
        };

    public:
        client_pool()
        {
        }
        instance get(const sd::string &key)
        {
            return instance{m_connections[key]};
        }
        awaitable<int> query()
        {
        }

    private:
        void erase(instance &)
        {
        }

    private:
        static thread_local std::map<std::string, client_list> m_connections;
    };
}