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
        opts.timeout = 8;
        opts.tries = 2;
        int optmask = ARES_OPT_SOCK_STATE_CB | ARES_OPT_TIMEOUT | ARES_OPT_TRIES;
        if (ares_init_options(&m_channel, &opts, optmask) != ARES_SUCCESS)
        {
            throw std::runtime_error("ares_init failed");
        }
    }

    Resolver::~Resolver()
    {
        ares_destroy(m_channel);
    }
    DNSCli &Resolver::__find(ares_socket_t _fd)
    {
        {
            auto it = m_clients.find(_fd);
            if (it != m_clients.end())
            {
                return it->second;
            }
        }
        {
            auto [it, ok] = m_clients.try_emplace(_fd, _fd, m_channel);
            if (!ok)
            {
                LOG_ERR("try_emplace failed");
                throw std::runtime_error("try_emplace failed");
            }
            return it->second;
        }
    }
    void Resolver::handler(void *data, ares_socket_t _fd, int readable, int writable)
    {
        auto _this = static_cast<Resolver *>(data);
        if (readable || writable)
        {
            _this->__init(_fd);
        }
        else
        {
            _this->__close(_fd);
        }
        LOG_CORE("handler %d %d:%d", _fd, readable, writable);
    }
    void Resolver::__init(int fd)
    {
        auto &cli = __find(fd);
        m_task << cli.send(nullptr, 0);
        m_task << cli.recv(nullptr, 0);
    }
    void Resolver::__close(int fd)
    {
        auto it = m_clients.find(fd);
        if (it != m_clients.end())
        {
            it->second.close();
            m_clients.erase(it);
        }
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
            LOG_ERR("ares_query failed: status=%d, error=%s", status, ares_strerror(status));
        }
    }

    coev::awaitable<int> Resolver::resolve(const std::string &hostname)
    {
        ares_gethostbyname(m_channel, hostname.c_str(), AF_INET, callback, this);
        auto r = co_await m_done.suspend();
        co_return r;
    }
    const std::string &Resolver::get_ip()
    {
        return m_ip;
    }
}