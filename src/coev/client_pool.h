/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#pragma once
#include <map>
#include <memory>
#include <cassert>
#include <utility>
#include "awaitable.h"
#include "queue.h"
#include "sleep_for.h"

namespace coev
{
    template <class CLI>
    class client_pool
    {
    public:
        struct CQ : CLI, queue
        {
            using CLI ::operator bool;
        };
        using NewClientFunc = awaitable<CQ *> (*)();
        struct CQP;
        struct QS : queue, std::enable_shared_from_this<QS>
        {
            uint32_t m_count = 0;
            uint32_t m_used = 0;
            uint32_t m_max = 0;
            async m_waiter;
            NewClientFunc m_func;

            awaitable<CQP> get();
            void release(queue *cq);
        };
        struct CQP
        {
            CQP() = default;
            CQP(std::shared_ptr<QS> _pool, CQ *_client) : m_pool(_pool), m_client(_client)
            {
            }
            CQP(CQP &&o) noexcept
            {
                m_pool = o.m_pool;
                m_client = std::exchange(o.m_client, nullptr);
            }
            CQP &operator=(CQP &&o) noexcept
            {
                if (this != &o)
                {
                    m_pool = o.m_pool;
                    m_client = std::exchange(o.m_client, nullptr);
                }
                return *this;
            }
            ~CQP()
            {
                if (m_client)
                {
                    m_pool->release(m_client);
                }
            }
            operator bool() const { return m_client != nullptr && *m_client; }
            CQ *operator->() { return m_client; }

            CQ *m_client = nullptr;
            std::shared_ptr<QS> m_pool = nullptr;
        };

    public:
        client_pool() = default;
        awaitable<CQP> get()
        {
            if (m_pool == nullptr)
            {
                init();
            }
            return m_pool->get();
        }
        void set(uint32_t _max, const NewClientFunc &func)
        {
            m_func = func;
            m_max = _max;
            if (m_pool == nullptr)
            {
                init();
            }
        }
        void stop()
        {
        }
        void init()
        {
            m_pool = std::shared_ptr<QS>(
                new QS,
                [](auto _qs)
                {
                    for (auto _conn = _qs->pop_front(); !_conn->empty(); _conn = _qs->pop_front())
                    {
                        delete _conn;
                        _qs->m_count--;
                    }
                    delete _qs;
                });
            m_pool->m_func = m_func;
            m_pool->m_max = m_max;
        }

    protected:
        static thread_local std::shared_ptr<QS> m_pool;
        NewClientFunc m_func;
        uint32_t m_max = 0;
    };
    template <class CLI>
    thread_local std::shared_ptr<typename client_pool<CLI>::QS> client_pool<CLI>::m_pool = {nullptr};

    template <class CLI>
    awaitable<typename client_pool<CLI>::CQP> client_pool<CLI>::QS::get()
    {
        while (true)
        {
            if (m_used == m_count && m_count == m_max)
            {
                co_await m_waiter.suspend();
            }
            if (m_count < m_max)
            {
                assert(m_func);
                auto cq = co_await m_func();
                push_back(cq);
                m_count++;
            }
            m_used++;
            auto cq = static_cast<client_pool<CLI>::CQ *>(pop_front());
            if (*cq)
            {
                co_return client_pool<CLI>::CQP(this->shared_from_this(), cq);
            }
            else
            {
                delete cq;
                m_count--;
                m_used--;
                co_await sleep_for(3.0);
            }
        }
    }

    template <class CLI>
    void client_pool<CLI>::QS::release(queue *cq)
    {
        if (cq)
        {
            push_back(cq);
        }
        m_used--;
        m_waiter.resume();
    }

}