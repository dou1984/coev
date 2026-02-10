#include "curl_cli.h"

namespace coev
{
    constexpr int READ = 0;
    constexpr int WRITE = 1;
    class init_curl_context
    {
    public:
        init_curl_context()
        {
            curl_global_init(CURL_GLOBAL_ALL);
        }
        ~init_curl_context()
        {
            curl_global_cleanup();
        }
    };
    struct CurlCli::Context : io_context
    {

        using io_context::io_context;
        auto read_waiter() { return m_r_waiter.suspend(); }
        auto write_waiter() { return m_w_waiter.suspend(); }
        auto id() { return m_fd; }
    };

    CurlCli::Instance::Instance()
    {
    }
    CurlCli::Instance::Instance(CURLM *m, CURL *c) : m_multi(m), m_curl(c)
    {
    }

    CurlCli::Instance::~Instance()
    {
        if (m_multi && m_curl)
        {
            curl_multi_remove_handle(m_multi, m_curl);
            m_multi = nullptr;
        }
        if (m_curl)
        {
            curl_easy_cleanup(m_curl);
            m_curl = nullptr;
        }
    }
    CurlCli::Instance::operator CURL *()
    {
        return m_curl;
    }
    CurlCli::Instance::operator bool() const
    {
        return m_curl && m_multi;
    }

    awaitable<int> CurlCli::Instance::action()
    {
        curl_easy_setopt(m_curl, CURLOPT_PRIVATE, this);
        auto r = curl_multi_add_handle(m_multi, m_curl);
        if (r != CURLM_OK)
        {
            LOG_ERR("curl_multi_add_handle failed");
            curl_easy_cleanup(m_curl);
            clear();
            co_return INVALID;
        }
        int running_handles = 0;
        curl_multi_socket_action(m_multi, CURL_SOCKET_TIMEOUT, 0, &running_handles);
        co_await m_done.suspend();
        co_return 0;
    }
    void CurlCli::Instance::done()
    {
        m_done.resume_next_loop();
    }
    void CurlCli::Instance::clear()
    {
        m_multi = nullptr;
        m_curl = nullptr;
    }

    CurlCli::CurlCli() : m_timer(100000000, 0)
    {
        singleton<init_curl_context>::instance();

        m_multi = curl_multi_init();
        if (m_multi != nullptr)
        {
            curl_multi_setopt(m_multi, CURLMOPT_SOCKETFUNCTION, CurlCli::cb_socket_event);
            curl_multi_setopt(m_multi, CURLMOPT_SOCKETDATA, this);
            curl_multi_setopt(m_multi, CURLMOPT_TIMERFUNCTION, CurlCli::cb_timer);
            curl_multi_setopt(m_multi, CURLMOPT_TIMERDATA, this);
            LOG_CORE("init success");
        }
        else
        {
            LOG_ERR("curl_multi_init error ");
        }

        m_task << [](auto _this) -> awaitable<void>
        {
            while (true)
            {
                co_await _this->m_timer.suspend();
                _this->action(CURL_SOCKET_TIMEOUT, 0);
            }
        }(this);
    }
    CurlCli::~CurlCli()
    {
        if (m_multi != nullptr)
        {
            curl_multi_cleanup(m_multi);
            LOG_CORE("curl client ");
        }
    }

    void CurlCli::check_multi_finished()
    {
        int pending;
        CURLMsg *message;
        while ((message = curl_multi_info_read(m_multi, &pending)))
        {
            if (message && message->msg == CURLMSG_DONE)
            {
                auto curl = message->easy_handle;
                const char *done_url = nullptr;
                curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &done_url);
                LOG_CORE("%s done", done_url);

                CurlCli::Instance *o = nullptr;
                curl_easy_getinfo(curl, CURLINFO_PRIVATE, &o);
                o->clear();
                o->done();
                curl_multi_remove_handle(m_multi, curl);
                curl_easy_cleanup(curl);
            }
        }
    }
    void CurlCli::action(curl_socket_t fd, int flags)
    {
        int running_handles;
        curl_multi_socket_action(m_multi, fd, flags, &running_handles);
        check_multi_finished();
    }
    void CurlCli::init_socket(curl_socket_t fd, int action)
    {
        auto __get_or_create = [this](auto fd) -> auto
        {
            auto it = m_clients.find(fd);
            if (it == m_clients.end())
            {
                auto context = m_clients[fd] = std::make_shared<Context>(fd);
                return context;
            }
            return it->second;
        };
        auto context = __get_or_create(fd);
        if (action == CURL_POLL_IN)
        {
            m_task << [](auto _this, auto context) -> coev::awaitable<void>
            {
                while (*context)
                {
                    co_await context->read_waiter();
                    if (context)
                    {
                        _this->action(context->id(), CURL_CSELECT_IN);
                    }
                }
            }(this, context);
        }
        if (action == CURL_POLL_OUT)
        {
            m_task << [](auto _this, auto context) -> coev::awaitable<void>
            {
                while (*context)
                {
                    co_await context->write_waiter();
                    if (context)
                    {
                        _this->action(context->id(), CURL_CSELECT_OUT);
                    }
                }
            }(this, context);
        }
    }
    int CurlCli::cb_socket_event(CURL *curl, curl_socket_t fd, int action, void *data, void *socketp)
    {
        LOG_CORE("cb_socket %d action %d data:%p %p", fd, action, data, socketp)
        auto _this = static_cast<CurlCli *>(data);
        switch (action)
        {
        case CURL_POLL_IN:
            _this->init_socket(fd, CURL_POLL_IN);
            break;
        case CURL_POLL_OUT:
            _this->init_socket(fd, CURL_POLL_OUT);
            break;
        case CURL_POLL_INOUT:
            _this->init_socket(fd, CURL_POLL_IN);
            _this->init_socket(fd, CURL_POLL_OUT);
            break;
        case CURL_POLL_REMOVE:
            _this->m_clients.erase(fd);
            break;
        default:
            LOG_ERR("cb_socket error %d", action);
            break;
        }
        return 0;
    }
    int CurlCli::cb_timer(CURLM *multi, long timeout_ms, void *userp)
    {
        LOG_CORE("timer %ld ", timeout_ms)
        auto _this = static_cast<CurlCli *>(userp);
        if (timeout_ms < 0)
        {
            _this->m_timer.stop();
        }
        else if (timeout_ms == 0)
        {
            _this->action(CURL_SOCKET_TIMEOUT, 0);
        }
        else
        {
            _this->m_timer.reset(double(timeout_ms) / 1000, 0);
        }
        return 0;
    }

    CurlCli::Instance CurlCli::get()
    {
        auto curl = curl_easy_init();
        return {m_multi, curl};
    }
}