/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "resolver.h"
#include "dns_cli.h"

namespace coev
{
    struct InitAres
    {
        InitAres()
        {
            ares_library_init(ARES_LIB_INIT_ALL);
        }
        ~InitAres()
        {
            ares_library_cleanup();
        }
    };

    Resolver::Resolver()
    {
        coev::singleton<InitAres>::instance();
        struct ares_options opts = {};
        opts.sock_state_cb = &Resolver::handler;
        opts.sock_state_cb_data = this;
        if (ares_init_options(&m_channel, &opts, ARES_OPT_SOCK_STATE_CB) != ARES_SUCCESS)
        {
            throw std::runtime_error("ares_init failed");
        }
    }

    Resolver::~Resolver()
    {
        ares_destroy(m_channel);
    }
    std::shared_ptr<DNSCli> Resolver::__find(ares_socket_t _fd)
    {
        auto it = m_clients.find(_fd);
        if (it == m_clients.end())
        {
            auto cli = std::make_shared<DNSCli>(_fd, m_channel);
            it = m_clients.emplace(_fd, cli).first;
        }
        return it->second;
    }
    void Resolver::handler(void *data, ares_socket_t _fd, int readable, int writable)
    {
        auto _this = static_cast<Resolver *>(data);
        if (readable || writable)
        {
            auto cli = _this->__find(_fd);
            _this->m_task << cli->send(nullptr, 0);
            _this->m_task << cli->recv(nullptr, 0);
        }
        else
        {
            auto it = _this->m_clients.find(_fd);
            if (it != _this->m_clients.end())
            {
                it->second->close();
                _this->m_clients.erase(it);
            }
        }
        LOG_CORE("handler %d %d:%d", _fd, readable, writable);
    }

    void Resolver::callback(void *arg, int status, int, struct hostent *host)
    {
        auto _this = static_cast<Resolver *>(arg);
        if (status == ARES_SUCCESS && host && host->h_addr_list[0])
        {
            _this->m_ip.resize(INET6_ADDRSTRLEN);
            inet_ntop(host->h_addrtype, host->h_addr_list[0], _this->m_ip.data(), INET6_ADDRSTRLEN);
            LOG_CORE("inet_ntop %s", _this->m_ip.c_str());
            _this->m_done.resume_next_loop();
        }
        else
        {
            LOG_CORE("ares_query failed %d", status);
        }
    }

    coev::awaitable<int> Resolver::resolve(const std::string &hostname)
    {
        ares_gethostbyname(m_channel, hostname.c_str(), AF_INET, callback, this);
        co_task _task;
        auto _done = _task << [this]() -> awaitable<void>
        { co_await m_done.suspend(); }();
        auto _timeout = _task << []() -> awaitable<void>
        { co_await sleep_for(15); }();
        auto _id = co_await _task.wait();
        LOG_CORE("resolve return %ld", _id);
        co_return _id == _done ? 0 : INVALID;
    }
    const std::string &Resolver::get_ip()
    {
        return m_ip;
    }

}