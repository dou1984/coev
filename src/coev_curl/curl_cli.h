/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <unordered_map>
#include <memory>
#include <coev/coev.h>
#include <curl/curl.h>

namespace coev
{
    class CurlCli final
    {
        struct Context;
        struct Instance;

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

    private:
        std::shared_ptr<Context> &__get_or_create(int fd);
        bool __exists(int fd) const;
        void __remove(int fd);
        awaitable<void> __time_waiter();
        awaitable<void> __read_waiter(curl_socket_t fd);
        awaitable<void> __write_waiter(curl_socket_t fd);

    private:
        CURLM *m_multi = nullptr;
        co_task m_task;
        co_timer m_timer;

    private:
        struct Instance
        {
            CURLM *m_multi = nullptr;
            CURL *m_curl = nullptr;
            co_async m_done;

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
        class Context : protected io_context
        {
        public:
            using io_context::io_context;
            using io_context::operator bool;
            using io_context::close;
            using io_context::fd;

            auto &__r_waiter() { return m_r_waiter; }
            auto &__w_waiter() { return m_w_waiter; }

            bool m_r_task = false;
            bool m_w_task = false;
        };
    };
}