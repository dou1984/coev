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
#include "gtid.h"
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
        struct Config
        {
            uint32_t m_max = 0;
            float m_delay = 1.0f;
            float m_short_delay = 0.1f;
            uint32_t m_wait_time = 15;
            std::string m_host;
            uint16_t m_port = 0;
        };
        struct ClientQueue : CLI, queue
        {
            using CLI::operator bool;
        };
        using CreateClientFunc = awaitable<ClientQueue *> (*)();
        struct ClientQueuePtr;
        struct SharedQueue : queue, std::enable_shared_from_this<SharedQueue>
        {
            uint32_t m_count = 0;
            uint32_t m_used = 0;
            async m_waiter;
            async m_closed;
            std::shared_ptr<Config> m_config;
            awaitable<ClientQueuePtr> get();
            awaitable<ClientQueue *> create_client();
            void release(queue *cq);
            ~SharedQueue() { clear(); }
            void clear();
            void init();
            auto get_client()
            {
                return static_cast<client_pool<CLI>::ClientQueue *>(pop_front());
            }
        };
        struct ClientQueuePtr
        {
            ClientQueuePtr() = default;
            ClientQueuePtr(std::shared_ptr<SharedQueue> _pool, ClientQueue *_client) : m_pool(_pool), m_client(_client) {}
            ClientQueuePtr(ClientQueuePtr &&o) noexcept
            {
                m_pool = o.m_pool;
                m_client = std::exchange(o.m_client, nullptr);
            }
            ClientQueuePtr &operator=(ClientQueuePtr &&o) noexcept
            {
                if (this != &o)
                {
                    m_pool = o.m_pool;
                    m_client = std::exchange(o.m_client, nullptr);
                }
                return *this;
            }
            ~ClientQueuePtr()
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
        awaitable<ClientQueuePtr> get() { return __get_sq()->get(); }
        void set(std::shared_ptr<Config> _conf)
        {
            m_config = _conf;
            __get_sq();
        }
        void stop()
        {
            std::lock_guard<std::mutex> _(m_mutex);
            for (auto [tid, qs] : m_pool)
            {
                qs->m_config->m_max = 0;
                while (auto c = qs->m_closed.pop_front())
                {
                    co_deliver::resume(static_cast<co_event *>(c));
                }
            }
            m_pool.clear();
        }
        std::shared_ptr<Config> get_config()
        {
            if (m_config == nullptr)
            {
                m_config = std::make_shared<Config>();
            }
            return m_config;
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
                sq->m_config = m_config;
                sq->init();
                it = m_pool.emplace(tid, sq).first;
            }
            return it->second;
        }
        std::mutex m_mutex;
        std::unordered_map<uint64_t, std::shared_ptr<SharedQueue>> m_pool;
        std::shared_ptr<Config> m_config;
    };

    template <class CLI>
    awaitable<typename client_pool<CLI>::ClientQueuePtr> client_pool<CLI>::SharedQueue::get()
    {
        while (true)
        {
            if (m_config->m_max == 0)
            {
                co_return client_pool<CLI>::ClientQueuePtr(this->shared_from_this(), nullptr);
            }
            if (m_used == m_count && m_count == m_config->m_max)
            {
                co_await m_waiter.suspend();
                continue;
            }
            if (m_count < m_config->m_max)
            {
                for (auto i = 0; i < m_config->m_wait_time; i++)
                {
                    auto cq = co_await create_client();
                    if (cq)
                    {
                        if (*cq)
                        {
                            m_count++;
                            m_used++;
                            co_return client_pool<CLI>::ClientQueuePtr(this->shared_from_this(), cq);
                        }
                        else
                        {
                            delete cq;
                        }
                    }
                    co_await sleep_for(m_config->m_delay);
                }
                co_return client_pool<CLI>::ClientQueuePtr(this->shared_from_this(), nullptr);
            }

            auto cq = get_client();
            if (*cq)
            {
                m_used++;
                co_return client_pool<CLI>::ClientQueuePtr(this->shared_from_this(), cq);
            }
            else
            {
                delete cq;
                m_count--;
                co_await sleep_for(m_config->m_short_delay);
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
        auto tid = gtid();
        while (auto cq = get_client())
        {
            LOG_CORE("tid:%ld delete %p", tid, cq);
            delete cq;
            m_count--;
        }
    }
    template <class CLI>
    void client_pool<CLI>::SharedQueue::init()
    {
        local<AutoReleaseTask>::instance() << [](auto _qs) -> awaitable<void>
        {
            auto tid = gtid();
            defer(LOG_CORE("AutoReleaseTask tid:%ld", tid));
            defer(_qs->clear());
            co_await _qs->m_closed.suspend();
        }(this->shared_from_this());
    }
    template <class CLI>
    awaitable<typename client_pool<CLI>::ClientQueue *> client_pool<CLI>::SharedQueue::create_client()
    {
        client_pool<CLI>::ClientQueue *cq = nullptr;
        try
        {
            cq = new client_pool<CLI>::ClientQueue();
            auto err = co_await cq->connect(m_config->m_host.c_str(), m_config->m_port);
            if (err == INVALID)
            {
                delete cq;
                co_return nullptr;
            }
            co_return cq;
        }
        catch (...)
        {
            LOG_CORE("[client_pool] connect error");
            if (cq)
            {
                delete cq;
            }
            co_return nullptr;
        }
    }
}