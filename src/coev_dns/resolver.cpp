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

    resolver::resolver()
    {
        coev::singleton<InitAres>::instance();
        struct ares_options opts = {};
        opts.sock_state_cb = &resolver::handler;
        opts.sock_state_cb_data = this;
        if (ares_init_options(&m_channel, &opts, ARES_OPT_SOCK_STATE_CB) != ARES_SUCCESS)
        {
            throw std::runtime_error("ares_init failed");
        }
    }

    resolver::~resolver()
    {
        ares_destroy(m_channel);
    }
    std::shared_ptr<dns_cli> resolver::__find(ares_socket_t _fd)
    {
        auto it = m_clients.find(_fd);
        if (it != m_clients.end())
        {
            return it->second;
        }
        else
        {
            auto cli = m_clients[_fd] = std::make_shared<dns_cli>(_fd, m_channel);
            return cli;
        }
    }
    void resolver::handler(void *data, ares_socket_t _fd, int readable, int writable)
    {
        auto _this = static_cast<resolver *>(data);
        if (readable || writable)
        {
            auto cli = _this->__find(_fd);
            _this->m_task << [](auto cli) -> awaitable<void>
            {
                co_await cli->send(nullptr, 0);
            }(cli);
            _this->m_task << [](auto cli) -> awaitable<void>
            {
                co_await cli->recv(nullptr, 0);
            }(cli);
        }
        else
        {
            _this->m_clients.erase(_fd);
        }
        LOG_CORE("handler %d %d:%d\n", _fd, readable, writable);
    }

    void resolver::callback(void *arg, int status, int, struct hostent *host)
    {
        auto _this = static_cast<resolver *>(arg);
        if (status == ARES_SUCCESS && host && host->h_addr_list[0])
        {
            _this->m_ip.resize(INET6_ADDRSTRLEN);
            inet_ntop(host->h_addrtype, host->h_addr_list[0], _this->m_ip.data(), INET6_ADDRSTRLEN);
            LOG_CORE("inet_ntop %s\n", _this->m_ip.c_str());
            _this->m_done.resume_next_loop();
        }
    }

    coev::awaitable<int> resolver::resolve(const std::string &hostname)
    {
        ares_gethostbyname(m_channel, hostname.c_str(), AF_INET, callback, this);
        co_task _task;
        auto _done = _task << [this]() -> awaitable<void>
        { co_await m_done.suspend(); }();
        auto _timeout = _task << []() -> awaitable<void>
        { co_await sleep_for(10); }();
        auto _id = co_await _task.wait();
        LOG_CORE("resolve return %ld\n", _id);
        co_return _id == _done ? 0 : INVALID;
    }
    std::string &resolver::get_ip()
    {
        return m_ip;
    }

}