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
        using client_queue = std::queue<CLI>;
        class instance
        {
            client_queue &m_connections;

        public:
            instance(client_queue &pool) : m_connections(pool)
            {
            }
            virtual ~instance()
            {
            }
        };

    public:
        client_pool()
        {
        }
        instance get(const std::string &key)
        {
            return instance{m_connections[key]};
        }

    private:
        void erase(instance &)
        {
        }

    private:
        static thread_local std::map<std::string, client_queue> m_connections;
    };
}