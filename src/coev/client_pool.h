/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#pragma once
#include <map>
#include <memory>
#include "awaitable.h"
#include "queue.h"

namespace coev
{
    template <class CLI>
    class client_pool
    {
        struct CQ;
        struct CQP;
        struct QS;
        using NewClientFunc = awaitable<CQ *>();

    protected:
        std::shared_ptr<QS> m_pool;
        NewClientFunc m_func;

    public:
        client_pool()
        {
            m_pool = std::shared_ptr<QS>(
                new QS,
                [](auto _qs)
                {
                    for (auto _conn = _qs->pop_front(); _conn; _conn = _qs->pop_front())
                    {
                        delete _conn;
                        _qs->m_count--;
                    }
                    delete _qs;
                });
        }

        awaitable<CQP> get()
        {
            assert(m_pool);
            return m_pool->get();
        }

        void set(const NewClientFunc &func)
        {
            m_func = func;
        }
        void set_max(uint32_t _max)
        {
            assert(m_pool);
            m_pool->m_max = _max;
        }

    protected:
        struct CQ : CLI, queue
        {
        };

        struct CQP
        {
            CQP() = default;
            CQP(std::shared_ptr<QS> &_pool, CQ *_client) : m_pool(_pool), m_client(_client)
            {
            }
            ~CQP()
            {
                if (m_client)
                {
                    m_pool->release(m_client);
                }
            }
            operator bool() const { return m_client != nullptr; }
            CQ *operator->() { return m_client; }

            CQ *m_client = nullptr;
            std::shared_ptr<QS> m_pool = nullptr;
        };
        struct QS : queue, std::enable_shared_from_this<QS>
        {
            uint32_t m_count = 0;
            uint32_t m_used = 0;
            uint32_t m_max = 0;
            async m_waiter;
            awaitable<CQP> get()
            {
                if (m_used == m_count && m_count == m_max)
                {
                    co_await m_waiter.suspend();
                }
                if (m_count < m_max)
                {
                    auto cq = m_func();
                    push_back(cq);
                    m_count++;
                }
                m_used++;
                co_return {shared_from_this(), pop_front()};
            }
            void release(queue *cq)
            {
                push_back(cq);
                m_used--;
                m_waiter.resume();
            }
        };
    };
}