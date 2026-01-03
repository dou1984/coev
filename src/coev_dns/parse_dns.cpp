#include <ares.h>
#include <array>
#include <memory>
#include <coev/coev.h>
#include <unordered_map>
#include <mutex>
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
    struct Context
    {
        std::string ip;
        std::mutex mtx;
        bool done = false;
    };
    class Resolver
    {
        ares_channel m_channel;
        Context m_context;
        std::unordered_map<ares_socket_t, std::shared_ptr<DnsCli>> m_clients;

    public:
        Resolver()
        {
            coev::singleton<InitAres>::instance();

            struct ares_options opts = {};
            opts.sock_state_cb = &Resolver::sock_state;
            opts.sock_state_cb_data = this;
            if (ares_init_options(&m_channel, &opts, ARES_OPT_SOCK_STATE_CB) != ARES_SUCCESS)
            {
                throw std::runtime_error("ares_init failed");
            }
        }

        ~Resolver()
        {
            ares_destroy(m_channel);
        }

        static void sock_state(void *data, ares_socket_t sockfd, int readable, int writable)
        {
            auto _this = static_cast<Resolver *>(data);
            if (!readable && !writable)
            {
                _this->m_clients.erase(sockfd);
            }
            else
            {
                auto cli = std::make_shared<DnsCli>(sockfd, _this->m_channel);
                _this->m_clients[sockfd] = cli;
                co_start << [](auto cli) -> awaitable<void>
                {
                    char ip[100];
                    co_await cli->recv(ip, sizeof(ip));
                    co_return;
                }(cli);
            }
            LOG_CORE("sock_state %d \n", sockfd);
        }

        static void dns_callback(void *arg, int status, int, struct hostent *host)
        {
            auto context = static_cast<Context *>(arg);
            std::lock_guard<std::mutex> lock(context->mtx);
            if (status == ARES_SUCCESS && host && host->h_addr_list[0])
            {
                context->ip.resize(INET6_ADDRSTRLEN);
                inet_ntop(host->h_addrtype, host->h_addr_list[0], context->ip.data(), INET6_ADDRSTRLEN);
            }
            context->done = true;
        }

        coev::awaitable<int> resolve(const std::string &hostname)
        {

            ares_gethostbyname(m_channel, hostname.c_str(), AF_INET, dns_callback, &m_context);
            while (true)
            {
                {
                    std::lock_guard<std::mutex> lock(m_context.mtx);
                    if (m_context.done)
                        break;
                }

                co_await coev::usleep_for(10000);
            }

            co_return 0;
        }

        std::string get_ip()
        {
            std::lock_guard<std::mutex> lock(m_context.mtx);
            return m_context.ip;
        }
    };

    coev::awaitable<int> parse_dns(const std::string &url, std::string &addr)
    {
        try
        {
            Resolver resolver;
            int r = co_await resolver.resolve(url);
            if (r == 0)
            {
                addr = resolver.get_ip();
                co_return 0;
            }
        }
        catch (...)
        {
            LOG_CORE("DNS resolve exception\n");
        }
        co_return INVALID;
    }
}