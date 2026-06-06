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
#include "finally.h"
#include "log.h"

namespace coev
{
    template <class CLI>
    class client_pool
    {
    public:
        using is_deleting = is_destroying;
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
            void *data;
        };
        struct client : queue, CLI
        {
            using CLI::CLI;
            using CLI::operator bool;
        };
        struct Instance;
        struct SQueue;

    public:
        client_pool() noexcept = default;
        ~client_pool() noexcept { stop(); }
        awaitable<int> get(Instance &ptr) noexcept
        {
            auto sq = __get_sq();
            co_return co_await sq->get_cli(ptr);
        }
        Instance instance() { return {}; }
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
                while (qs->m_closed.resume())
                {
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
        std::shared_ptr<SQueue> __get_sq() noexcept
        {
            std::lock_guard<std::mutex> _(m_mutex);
            auto tid = gtid();
            auto it = m_pool.find(tid);
            if (it == m_pool.end())
            {
                auto sq = std::make_shared<SQueue>();
                sq->m_config = m_config;
                sq->init();
                it = m_pool.emplace(tid, sq).first;
            }
            return it->second;
        }
        std::mutex m_mutex;
        std::unordered_map<uint64_t, std::shared_ptr<SQueue>> m_pool;
        std::shared_ptr<Config> m_config;

    public:
        struct SQueue : queue, std::enable_shared_from_this<SQueue>
        {
            friend class client_pool;

            awaitable<int> get_cli(Instance &) noexcept;
            awaitable<client *> create() noexcept;
            void del(client *) noexcept;
            void release(queue *cq) noexcept;
            ~SQueue() noexcept
            {
                m_config->max_connections = 0;
                while (m_waiter.resume(INVALID))
                {
                }
                clear();
            }
            void clear() noexcept;
            void init() noexcept;
            auto get() noexcept
            {
                return static_cast<client *>(pop_front());
            }

        private:
            std::shared_ptr<Config> m_config;
            uint32_t m_connected = 0;
            uint32_t m_connecting = 0;
            uint32_t m_used = 0;
            co_async m_waiter;
            co_async m_closed;
        };
        struct Instance
        {
            Instance() noexcept = default;
            Instance(std::shared_ptr<SQueue> _pool, client *_client) noexcept : m_pool(_pool), m_client(_client) {}
            Instance(Instance &&o) noexcept
            {
                m_pool = std::move(o.m_pool);
                m_client = std::exchange(o.m_client, nullptr);
            }
            Instance &operator=(Instance &&o) noexcept
            {
                if (this != &o)
                {
                    m_pool = std::move(o.m_pool);
                    m_client = std::exchange(o.m_client, nullptr);
                }
                return *this;
            }
            Instance(const Instance &) = delete;
            Instance &operator=(const Instance &) = delete;
            ~Instance() noexcept
            {
                auto _client = std::exchange(m_client, nullptr);
                if (_client)
                {
                    if (!local<is_deleting>::instance())
                    {
                        if (m_pool)
                        {
                            if (m_pool->m_config->max_connections > 0)
                            {
                                m_pool->release(_client);
                                return;
                            }
                        }
                    }
                    delete _client;
                }
            }
            operator bool() const noexcept { return m_client != nullptr && *m_client; }
            client *operator->() noexcept { return m_client; }

            client *m_client = nullptr;
            std::shared_ptr<SQueue> m_pool = nullptr;
        };
    };

    template <class CLI>
    awaitable<int> client_pool<CLI>::SQueue::get_cli(client_pool<CLI>::Instance &ptr) noexcept
    {
        assert(!ptr); // 需要传入空instance
        while (true)
        {
            if (m_config->max_connections == 0)
            {
                co_return INVALID;
            }
            if (m_used == m_connected && (m_config->max_connections == (m_connected + m_connecting)))
            {
                auto r = co_await m_waiter.suspend();
                if (r == INVALID)
                {
                    co_return r;
                }
                continue;
            }
            if ((m_connected + m_connecting) < m_config->max_connections)
            {
                m_connecting++;
                finally(m_connecting--);
                for (auto i = 0; i < m_config->retry_count; i++)
                {
                    if (auto cq = co_await create())
                    {
                        if (*cq)
                        {
                            m_connected++;
                            m_used++;
                            ptr.m_pool = this->shared_from_this();
                            ptr.m_client = cq;
                            co_return 0;
                        }
                        else
                        {
                            del(cq);
                        }
                    }
                    co_await sleep_for(m_config->retry_time);
                }
                co_return INVALID;
            }
            auto cq = get();
            if (cq == nullptr)
            {
                continue;
            }
            if (*cq)
            {
                m_used++;
                ptr.m_pool = this->shared_from_this();
                ptr.m_client = cq;
                co_return 0;
            }
            else
            {
                del(cq);
                co_await sleep_for(m_config->quick_retry_time);
            }
        }
    }
    template <class CLI>
    void client_pool<CLI>::SQueue::release(queue *cq) noexcept
    {
        if (cq)
        {
            push_back(cq);
        }
        assert(m_used > 0);
        m_used--;
        m_waiter.resume();
    }
    template <class CLI>
    void client_pool<CLI>::SQueue::clear() noexcept
    {
        for (auto cq = get(); cq; cq = get())
        {
            del(cq);
        }
    }
    template <class CLI>
    void client_pool<CLI>::SQueue::init() noexcept
    {
        co_start << [](auto _qs) -> awaitable<void>
        {
            auto tid = gtid();
            finally(_qs->clear());
            co_await _qs->m_closed.suspend();
        }(this->shared_from_this());
    }
    template <class CLI>
    void client_pool<CLI>::SQueue::del(client *cq) noexcept
    {
        try
        {
            if (m_connected > 0)
            {
                m_connected--;
            }
            std::lock_guard<is_deleting> _(local<is_deleting>::instance());
            delete cq;
        }
        catch (...)
        {
        }
    }
    template <class CLI>
    awaitable<typename client_pool<CLI>::client *> client_pool<CLI>::SQueue::create() noexcept
    {
        client_pool<CLI>::client *cq = nullptr;
        int r = INVALID;
        try
        {
            cq = new client_pool<CLI>::client(m_config);
            finally(
                {
                    if (cq && r == INVALID)
                    {
                        delete cq;
                    }
                });
            r = co_await cq->connect();
            if (r == INVALID)
            {
                co_return nullptr;
            }
            co_return cq;
        }
        catch (...)
        {
            co_return nullptr;
        }
    }
}