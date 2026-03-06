/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include "server.h"
namespace coev::http
{
    const int max_wait_time = 60;
    void server::set_router(const std::string &path, const route &_route)
    {
        m_router.emplace(path, _route);
    }
    awaitable<void> server::worker()
    {
        while (valid())
        {
            addrInfo h;
            auto fd = co_await accept(h);
            if (fd == INVALID)
            {
                LOG_CORE("accept error %s", strerror(errno));
                continue;
            }
            m_task << __handle(fd, h);
        }
    }
    awaitable<void> server::__handle(int fd, addrInfo &h)
    {
        io_context io(fd);
        http::session r;
        http::request req;
        auto ok = co_await r.get_request(io, req);
        if (ok != 0)
        {
            LOG_CORE("parse http request error %s", req.status.c_str());
            std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            co_await io.send(response);
            co_return;
        }
        co_task _task;
        auto url = req.url.substr(0, req.url.find('?'));
        if (auto it = m_router.find(url); it != m_router.end())
        {
            _task << it->second(io, req);
            if (_task.empty())
            {
                LOG_CORE("task is already finished");
                co_return;
            }
            auto _timeout = _task << []() -> awaitable<void>
            {
                co_await sleep_for(max_wait_time);
            }();
            auto _result = co_await _task.wait();
            if (_result == _timeout)
            {
                std::string response = "HTTP/1.1 408 Request Timeout\r\n\r\n";
                co_await io.send(response);
            }
        }
        else
        {
            std::string response = "HTTP/1.1 404 Not Found\r\n\r\n";
            co_await io.send(response);
        }
    }

}