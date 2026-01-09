#pragma once
#include <unordered_map>
#include <coev/coev.h>
#include <curl/curl.h>

namespace coev
{
    class CurlCli
    {
        struct Context;
        class Instance
        {
            CURLM *m_multi = nullptr;
            CURL *m_curl = nullptr;
            async m_done;

        public:
            Instance();
            Instance(CURLM *, CURL *);
            virtual ~Instance();
            operator CURL *();
            operator bool() const;
            awaitable<int> action();
            void done();
            void clear();
            template <class T>
            CURLcode setopt(CURLoption opt, T &&ptr)
            {
                return m_curl ? curl_easy_setopt(m_curl, opt, std::forward<T>(ptr)) : CURLE_FAILED_INIT;
            }
        };

    public:
        CurlCli();
        ~CurlCli();
        Instance get();

    protected:
        std::unordered_map<curl_socket_t, std::shared_ptr<Context>> m_clients;
        static int cb_socket_event(CURL *curl, curl_socket_t s, int action, void *userp, void *socketp);
        static int cb_timer(CURLM *multi, long timeout_ms, void *userp);
        void action(curl_socket_t fd, int flags);
        void check_multi_finished();
        void init_socket(curl_socket_t, int);

    protected:
        CURLM *m_multi = nullptr;
        co_task m_task;
        co_timer m_timer;
    };
}