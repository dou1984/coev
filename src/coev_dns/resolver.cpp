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
    }
    awaitable<void> Resolver::__send(int fd)
    {
        auto &cli = __find(fd);
        co_await cli.send(nullptr, 0);
    }
    awaitable<void> Resolver::__recv(int fd)
    {
        auto &cli = __find(fd);
        co_await cli.recv(nullptr, 0);
        m_clients.erase(fd);
    }
    void Resolver::__init(int _fd)
    {
        m_task << __send(_fd);
        m_task << __recv(_fd);
    }
    void Resolver::__close(int fd)
    {
        auto it = m_clients.find(fd);
        if (it != m_clients.end())
        {
            it->second.close();
        }
    }

    void Resolver::callback(void *arg, int status, int, struct ares_addrinfo *res)
    {
        finally(ares_freeaddrinfo(res));
        auto _this = static_cast<Resolver *>(arg);
        if (status == ARES_SUCCESS && res)
        {
            auto node = res->nodes;
            while (node)
            {
                if (node->ai_family == AF_INET || node->ai_family == AF_INET6)
                {
                    _this->m_ip.resize(INET6_ADDRSTRLEN);
                    struct sockaddr_in *addr_in = nullptr;
                    struct sockaddr_in6 *addr_in6 = nullptr;

                    if (node->ai_family == AF_INET)
                    {
                        addr_in = (struct sockaddr_in *)node->ai_addr;
                        inet_ntop(AF_INET, &addr_in->sin_addr, _this->m_ip.data(), INET6_ADDRSTRLEN);
                    }
                    else
                    {
                        addr_in6 = (struct sockaddr_in6 *)node->ai_addr;
                        inet_ntop(AF_INET6, &addr_in6->sin6_addr, _this->m_ip.data(), INET6_ADDRSTRLEN);
                    }
                    break;
                }
                node = node->ai_next;
            }
        }
        _this->m_done.resume_next_loop();
    }

    coev::awaitable<int> Resolver::resolve(const std::string &hostname)
    {
        struct ares_addrinfo_hints hints = {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        ares_getaddrinfo(m_channel, hostname.c_str(), nullptr, &hints, callback, this);
        co_await m_done.suspend();
        co_return 0;
    }
    const std::string &Resolver::get_ip()
    {
        return m_ip;
    }
}