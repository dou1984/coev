/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_nghttp/co_nghttp2.h>
#include <cstring>
#include <atomic>

using namespace coev;
ssl::manager g_cli_mgr(ssl::manager::TLS_CLIENT);
ssl::manager g_srv_mgr(ssl::manager::TLS_SERVER);

char hi[] = R"(helloworld)";

int worker_num = 4;      // 单核测试
int coroutine_num = 1000;  // 单协程测试瓶颈
int max_connection = 200; // 单连接
int max_query = 10000;  // 增加请求数以便统计

coev::pool::nghttp2::Http2 http2;
coev::pool::server_pool<nghttp2::server> server;

std::atomic<uint64_t> g_request_count{0};
std::atomic<uint64_t> g_error_count{0};
std::atomic<uint64_t> g_total_bytes{0};

awaitable<void> echo(nghttp2::session &ctx, nghttp2::request &request)
{
    nghttp2::header ngh;
    ngh.push_back(":status", "200");

    char large_payload[512];
    memset(large_payload, 'A', sizeof(large_payload));

    auto err = ctx.reply(request.id(), ngh, large_payload, sizeof(large_payload));

    if (err == INVALID)
    {
        g_error_count++;
        co_return;
    }

    g_request_count++;
};
awaitable<void> proc_server()
{
    auto &srv = server.get();
    srv.set_router("/echo", echo);
    co_await srv.dispatch(nullptr);
    co_return;
}
awaitable<void> proc_client()
{
    auto start_time = std::chrono::steady_clock::now();

    co_task task;
    for (auto w = 0; w < coroutine_num; w++)
    {
        task << [w]() -> awaitable<void>
        {
            int local_errors = 0;

            for (auto i = 0; i < max_query; i++)
            {
                coev::pool::nghttp2::Http2::instance c;
                auto r = co_await http2.get(c);
                if (r == INVALID)
                {
                    g_error_count++;
                    local_errors++;
                    co_return;
                }

                nghttp2::header ngh;
                ngh.push_back(":method", "GET");
                ngh.push_back(":path", "/echo");
                ngh.push_back(":scheme", "https");
                ngh.push_back(":authority", "coev");
                ngh.push_back("accept", "*/*");
                ngh.push_back("user-agent", "nghttp2/" NGHTTP2_VERSION);
                nghttp2::response res;
                std::string hi = std::to_string(i);
                auto err = co_await c->query(ngh, hi.data(), hi.size(), res);
                if (err == INVALID)
                {
                    g_error_count++;
                    local_errors++;
                    co_return;
                }

                g_request_count++;
                g_total_bytes += res.body().size();
            }
        }();
    }
    co_await task.wait();

    auto end_time = std::chrono::steady_clock::now();
    auto total_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    auto qps = g_request_count.load() * 1000.0 / (total_ms + 1);
    LOG_INFO("==========================================");
    LOG_INFO("压力测试完成");
    LOG_INFO("总请求数：%lu", g_request_count.load());
    LOG_INFO("错误数：%lu", g_error_count.load());
    LOG_INFO("总数据量：%lu bytes (%.2f MB)", g_total_bytes.load(), g_total_bytes.load() / 1024.0 / 1024.0);
    LOG_INFO("总耗时：%ld ms", total_ms);
    LOG_INFO("QPS: %.2f", qps);
    LOG_INFO("==========================================");

    co_return;
}
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LOG_ERR("usage: %s server|client", argv[0]);
        return 1;
    }

    set_log_level(LOG_LEVEL_CORE);
    // set_log_level(LOG_LEVEL_DEBUG);
    // set_log_level(LOG_LEVEL_ERROR);
    // set_log_level(LOG_LEVEL_INFO);

    if (strcmp(argv[1], "server") == 0)
    {
        server.start("0.0.0.0", 9999);
        // g_srv_mgr.use_certificate_file("./certs/server.crt");
        // g_srv_mgr.use_private_key_file("./certs/server.key");
        runnable::instance()
            .start(worker_num, proc_server)
            .end([]()
                 { server.stop(); });
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        auto config = http2.get_config();
        config->host = "127.0.0.1";
        config->port = 9999;
        // config->data = g_cli_mgr.get();
        config->max_connections = max_connection;
        http2.set(config);
        runnable::instance()
            .start(worker_num, proc_client)
            .end([]()
                 { http2.stop(); });
    }

    return 0;
}