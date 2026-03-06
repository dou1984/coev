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
#include <string>
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
            uint32_t max_connections = 10;
            float retry_time = 1.0f;
            float quick_retry_time = 0.1f;
            uint32_t retry_count = 15;
            std::string host;
            uint16_t port = 0;
            std::string username;
            std::string password;
            std::map<std::string, std::string> options;
        };
        struct client : queue, CLI
        {
            using CLI::CLI;
            using CLI::operator bool;
        };
        using is_deleting = is_destroying;
        struct instance;
        struct shared_queue : queue, std::enable_shared_from_this<shared_queue>
        {
            awaitable<int> get(instance &) noexcept;
            awaitable<client *> create_client() noexcept;
            void delete_client(client *) noexcept;
            void release(queue *cq) noexcept;
            ~shared_queue() noexcept { clear(); }
            void clear() noexcept;
            void init() noexcept;
            auto get_client() noexcept { return static_cast<client *>(pop_front()); }

            std::shared_ptr<Config> m_config;
            uint32_t m_count = 0;
            uint32_t m_used = 0;
            async m_waiter;
            async m_closed;
        };
        struct instance
        {
            instance() noexcept = default;
            instance(std::shared_ptr<shared_queue> _pool, client *_client) noexcept : m_pool(_pool), m_client(_client) {}
            instance(instance &&o) noexcept
            {
                m_pool = o.m_pool;
                m_client = std::exchange(o.m_client, nullptr);
            }
            instance &operator=(instance &&o) noexcept
            {
                if (this != &o)
                {
                    m_pool = o.m_pool;
                    m_client = std::exchange(o.m_client, nullptr);
                }
                return *this;
            }
            instance(const instance &) = delete;
            instance &operator=(const instance &) = delete;
            ~instance() noexcept
            {
                if (!local<is_deleting>::instance())
                {
                    if (m_pool && m_client)
                    {
                        LOG_CORE("delete client %p", m_client);
                        auto _client = std::exchange(m_client, nullptr);
                        m_pool->release(_client);
                    }
                }
            }
            operator bool() const noexcept { return m_client != nullptr && *m_client; }
            client *operator->() noexcept { return m_client; }

            client *m_client = nullptr;
            std::shared_ptr<shared_queue> m_pool = nullptr;
        };
        using auto_release_task = guard::co_task;

    public:
        client_pool() noexcept = default;
        awaitable<int> get(instance &ptr) noexcept { return __get_sq()->get(ptr); }
        void set(std::shared_ptr<Config> _conf) noexcept
        {
            m_config = _conf;
            __get_sq();
        }
        void stop() noexcept
        {
            std::lock_guard<std::mutex> _(m_mutex);
            for (auto [tid, qs] : m_pool)
            {
                qs->m_config->max_connections = 0;
                while (auto c = qs->m_closed.pop_front())
                {
                    co_deliver::resume(static_cast<co_event *>(c));
                }
            }
            m_pool.clear();
        }
        std::shared_ptr<Config> get_config() noexcept
        {
            if (m_config == nullptr)
            {
                m_config = std::make_shared<Config>();
            }
            return m_config;
        }

    protected:
        std::shared_ptr<shared_queue> __get_sq() noexcept
        {
            std::lock_guard<std::mutex> _(m_mutex);
            auto tid = gtid();
            auto it = m_pool.find(tid);
            if (it == m_pool.end())
            {
                auto sq = std::make_shared<shared_queue>();
                sq->m_config = m_config;
                sq->init();
                it = m_pool.emplace(tid, sq).first;
            }
            return it->second;
        }
        std::mutex m_mutex;
        std::unordered_map<uint64_t, std::shared_ptr<shared_queue>> m_pool;
        std::shared_ptr<Config> m_config;
    };

    template <class CLI>
    awaitable<int> client_pool<CLI>::shared_queue::get(client_pool<CLI>::instance &ptr) noexcept
    {
        ptr.m_pool = nullptr;
        ptr.m_client = nullptr;
        while (true)
        {
            if (m_config->max_connections == 0)
            {
                co_return INVALID;
            }
            if (m_used == m_count && m_count == m_config->max_connections)
            {
                co_await m_waiter.suspend();
                continue;
            }
            if (m_count < m_config->max_connections)
            {
                for (auto i = 0; i < m_config->retry_count; i++)
                {
                    auto cq = co_await create_client();
                    if (cq)
                    {
                        if (*cq)
                        {
                            m_count++;
                            m_used++;
                            ptr.m_pool = this->shared_from_this();
                            ptr.m_client = cq;
                            co_return 0;
                        }
                        else
                        {
                            delete_client(cq);
                        }
                    }
                    co_await sleep_for(m_config->retry_time);
                }
                co_return INVALID;
            }

            auto cq = get_client();
            if (*cq)
            {
                m_used++;
                ptr.m_pool = this->shared_from_this();
                ptr.m_client = cq;
                co_return 0;
            }
            else
            {
                delete_client(cq);
                m_count--;
                co_await sleep_for(m_config->quick_retry_time);
            }
        }
    }
    template <class CLI>
    void client_pool<CLI>::shared_queue::release(queue *cq) noexcept
    {
        if (cq)
        {
            push_back(cq);
        }
        m_used--;
        assert(m_used >= 0);
        m_waiter.resume();
    }
    template <class CLI>
    void client_pool<CLI>::shared_queue::clear() noexcept
    {
        auto tid = gtid();
        LOG_CORE("tid:%ld clear %d", tid, m_count);
        for (auto cq = get_client(); cq; cq = get_client())
        {
            LOG_CORE("tid:%ld delete %p", tid, cq);
            delete_client(cq);
            m_count--;
        }
    }
    template <class CLI>
    void client_pool<CLI>::shared_queue::init() noexcept
    {
        local<auto_release_task>::instance() << [](auto _qs) -> awaitable<void>
        {
            auto tid = gtid();
            defer(_qs->clear());
            defer(LOG_CORE("auto_release_task tid:%ld", tid));
            co_await _qs->m_closed.suspend();
        }(this->shared_from_this());
    }
    template <class CLI>
    void client_pool<CLI>::shared_queue::delete_client(client *cq) noexcept
    {
        std::lock_guard<is_deleting> _(local<is_deleting>::instance());
        delete cq;
    }
    template <class CLI>
    awaitable<typename client_pool<CLI>::client *> client_pool<CLI>::shared_queue::create_client() noexcept
    {
        try
        {
            auto cq = new client_pool<CLI>::client(m_config);
            auto err = co_await cq->connect();
            if (err == INVALID)
            {
                delete_client(cq);
                co_return nullptr;
            }
            co_return cq;
        }
        catch (...)
        {
            LOG_CORE("client_pool connect error");
            co_return nullptr;
        }
    }
}