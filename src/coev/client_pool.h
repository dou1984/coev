/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#pragma once
#include <queue>
#include "awaitable.h"
#include "queue.h"

namespace coev
{
    template <class CLI>
    class client_pool
    {
        using client_list = queue<CLI>;
        class instance
        {
            client_list &m_connections;
            async& m_done;
          
        public:
            instance(client_list &_list, async& _done) : m_connections(_list) ,m_done(_done) {}
            virtual ~instance() 

            CLI&  operator co_await()
            {
                if (m_connections.empty())
                {
                    co_await m_done; 
                }
                if (m_connections.empty())
                {
                }

            }
        };

    public:
        client_pool() = default;
       
        instance get(const std::string &key)
        {
            return instance{m_connections[key], m_done[key]};
        }
        template <class Config>
        void init(const std::string& key,  Config& conf)
        {
            m_connections.emplace(key, conf);
            m_done[key]; 
        }   

    protected:
         std::map<std::string, client_list> m_connections;
         std::map<std::string, async > m_done;
    };
}