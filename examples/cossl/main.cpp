/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_ssl/ssl.h>

using namespace coev;
using namespace coev::ssl;

static manager g_srv_mgr(manager::TLS_SERVER);
static manager g_cli_mgr(manager::TLS_CLIENT);

coev::pool::ssl::client cli;
coev::pool::server_pool<tcp::server> _pool;
auto send_times = 1000000000;
awaitable<void> test_ssl_context()
{

    LOG_DBG("server started");
    while (true)
    {
        addrInfo addr;
        auto fd = co_await _pool.get().accept(addr);
        LOG_DBG("accepted %d from %s:%d", fd, addr.ip, addr.port);

        if (fd == INVALID)
        {
            LOG_ERR("accept failed fd:%d", fd);
            continue;
        }
        co_start << [](auto fd) -> awaitable<void>
        {
            context ctx(fd, g_srv_mgr.get());
            defer(LOG_DBG("finished fd:%d", fd));
            int err = co_await ctx.do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake failed fd:%d", fd);
                co_return;
            }
            while (true)
            {
                char buffer[1024];
                int r = co_await ctx.recv(buffer, sizeof(buffer));
                if (r == INVALID)
                {
                    LOG_ERR("recv failed fd:%d", fd);
                    co_return;
                }
                LOG_DBG("recv from fd:%d %.*s", fd, r, buffer);
                r = co_await ctx.send("hello world", strlen("hello world") + 1);
                if (r == INVALID)
                {
                    LOG_ERR("send failed fd:%d", fd);
                    co_return;
                }
            }
        }(fd);
    }
}
awaitable<void> test_ssl_client()
{
    // 性能统计
    auto start = std::chrono::steady_clock::now();      
    
 
    const int TASK_COUNT = 20; 
    const int REQUESTS_PER_TASK = send_times / TASK_COUNT;
    
    LOG_ERR("Starting %d concurrent tasks, %d requests each...", TASK_COUNT, REQUESTS_PER_TASK);
    
    co_task _task;
    // 启动多个并发任务
    for (int t = 0; t < TASK_COUNT; t++)
    {
        _task << [REQUESTS_PER_TASK](auto t) -> awaitable<void> {
            coev::pool::ssl::client::instance c;
            auto err = co_await cli.get(c);
            if (err == INVALID)
            {
                LOG_ERR("Task %d: get connection failed", t);            
                co_return;
            }
            
            for (int i = 0; i < REQUESTS_PER_TASK; i++)
            {
                auto buf = "hello worldhello worldhello worldhello worldhello worldhello worldhello worldhelloworldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello world";
                int size = strlen(buf) + 1;
                int r = co_await c->send(buf, size);
                if (r == INVALID)
                {
                    LOG_ERR("Task %d: send failed", t);                 
                    co_return;
                }             
                
                char buffer[4096];
                r = co_await c->recv(buffer, sizeof(buffer));
                if (r == INVALID)
                {
                    LOG_ERR("Task %d: recv failed", t);                 
                    co_return;
                }               
            }    
      
        }(t);
    }
  
    co_await _task.wait_all();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double seconds = duration.count() / 1000.0;
    double qps = send_times / seconds;
   
    
    LOG_ERR("=== Performance Report ===");
    LOG_ERR("Total requests: %d", send_times);
    LOG_ERR("Duration: %.2f seconds", seconds);
    LOG_ERR("QPS: %.2f", qps);
    LOG_ERR("========================");
}
int main(int argc, char **argv)
{
    // set_log_level(LOG_LEVEL_DEBUG);
    set_log_level(LOG_LEVEL_ERROR);
    // set_log_level(LOG_LEVEL_CORE);
    if (argc < 2)
    {
        LOG_ERR("usage: %s [server|client]", argv[0]);
        exit(INVALID);
    }
    if (strcmp(argv[1], "server") == 0)
    {
        g_srv_mgr.use_certificate_file("./certs/server/server.crt");
        g_srv_mgr.use_private_key_file("./certs/server/server.key");
        _pool.start("0.0.0.0", 9999);
        runnable::instance()
            .start(test_ssl_context)
            .end([]()
                 { _pool.stop(); });
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        auto conf = cli.get_config();
        conf->host = "0.0.0.0";
        conf->port = 9999;
        conf->max_connections = 40;
        conf->data = g_cli_mgr.get();

        runnable::instance()
            .start(test_ssl_client)
            .end([]()
                 { cli.stop(); });
    }
    else
    {
        LOG_ERR("usage: %s [server|client]", argv[0]);
    }

    return 0;
}