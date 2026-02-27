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
#include "co_deliver.h"
#include "defer.h"

namespace coev
{
    template <class CLI>
    class client_pool
    {
    public:
        struct ClientQueue : CLI, queue
        {
            using CLI::operator bool;
        };
        using CreateClientFunc = awaitable<ClientQueue *> (*)();
        struct ClientQueueWrapper;
        struct SharedQueue : queue, std::enable_shared_from_this<SharedQueue>
        {
            uint32_t m_count = 0;
            uint32_t m_used = 0;
            uint32_t m_max = 0;
            float m_delay = 1.0f;
            async m_waiter;
            async m_closed;
            CreateClientFunc m_create;
            awaitable<ClientQueueWrapper> get();
            void release(queue *cq);
            ~SharedQueue() { clear(); }
            void clear();
            void init();
        };
        struct ClientQueueWrapper
        {
            ClientQueueWrapper() = default;
            ClientQueueWrapper(std::shared_ptr<SharedQueue> _pool, ClientQueue *_client) : m_pool(_pool), m_client(_client) {}
            ClientQueueWrapper(ClientQueueWrapper &&o) noexcept
            {
                m_pool = o.m_pool;
                m_client = std::exchange(o.m_client, nullptr);
            }
            ClientQueueWrapper &operator=(ClientQueueWrapper &&o) noexcept
            {
                if (this != &o)
                {
                    m_pool = o.m_pool;
                    m_client = std::exchange(o.m_client, nullptr);
                }
                return *this;
            }
            ~ClientQueueWrapper()
            {
                if (m_client)
                {
                    m_pool->release(m_client);
                }
            }
            operator bool() const { return m_client != nullptr && *m_client; }
            ClientQueue *operator->() { return m_client; }

            ClientQueue *m_client = nullptr;
            std::shared_ptr<SharedQueue> m_pool = nullptr;
        };
        using AutoReleaseTask = co_task;

    public:
        client_pool() = default;
        awaitable<ClientQueueWrapper> get() { return __get_sq()->get(); }
        void set(uint32_t _max, const CreateClientFunc &func)
        {
            m_create = func;
            m_max = _max;
            __get_sq();
        }
        void stop()
        {
            std::lock_guard<std::mutex> _(m_mutex);
            for (auto &[tid, _qs] : m_pool)
            {
                _qs->m_max = 0;
                for (auto c = _qs->m_closed.pop_front(); !c->empty(); c = _qs->m_closed.pop_front())
                {
                    co_deliver::resume(static_cast<co_event *>(c));
                }
            }
            m_pool.clear();
        }

    protected:
        auto __get_sq()
        {
            std::lock_guard<std::mutex> _(m_mutex);
            auto tid = gtid();
            auto it = m_pool.find(tid);
            if (it == m_pool.end())
            {
                auto sq = std::make_shared<SharedQueue>();
                sq->m_create = m_create;
                sq->m_max = m_max;
                sq->init();
                it = m_pool.emplace(tid, sq).first;
            }
            return it->second;
        }

        std::unordered_map<uint64_t, std::shared_ptr<SharedQueue>> m_pool;
        std::mutex m_mutex;
        uint32_t m_max = 0;
        CreateClientFunc m_create;
    };

    template <class CLI>
    awaitable<typename client_pool<CLI>::ClientQueueWrapper> client_pool<CLI>::SharedQueue::get()
    {
        while (true)
        {
            if (m_used == m_count && m_count == m_max)
            {
                co_await m_waiter.suspend();
            }
            if (m_count < m_max)
            {
                while (true)
                {
                    assert(m_create);
                    auto cq = co_await m_create();
                    if (cq)
                    {
                        push_back(cq);
                        m_count++;
                        break;
                    }
                    else
                    {
                        co_await sleep_for(m_delay);
                    }
                }
            }

            auto cq = static_cast<client_pool<CLI>::ClientQueue *>(pop_front());
            if (*cq)
            {
                m_used++;
                co_return client_pool<CLI>::ClientQueueWrapper(this->shared_from_this(), cq);
            }
            else
            {
                delete cq;
                m_count--;
                co_await sleep_for(m_delay);
            }
        }
    }

    template <class CLI>
    void client_pool<CLI>::SharedQueue::release(queue *cq)
    {
        if (cq)
        {
            push_back(cq);
        }
        m_used--;
        m_waiter.resume();
    }

    template <class CLI>
    void client_pool<CLI>::SharedQueue::clear()
    {
        for (auto cq = pop_front(); !cq->empty(); cq = pop_front())
        {
            delete cq;
            m_count--;
        }
    }
    template <class CLI>
    void client_pool<CLI>::SharedQueue::init()
    {
        local<AutoReleaseTask>::instance() << [](auto _this) -> awaitable<void>
        {
            defer(LOG_CORE("AutoReleaseTask"));
            defer(_this->clear());
            co_await _this->m_closed.suspend();
        }(this->shared_from_this());
    }
}