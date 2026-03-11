/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_nghttp/co_nghttp2.h>

using namespace coev;
ssl::manager g_cli_mgr(ssl::manager::TLS_CLIENT);
ssl::manager g_srv_mgr(ssl::manager::TLS_SERVER);

char hi[] = R"(helloworld)";
int worker_num = 4;
int coroutine_num = 10;
int max_connection = 50;
coev::pool::nghttp2::Http2 http2;
coev::pool::server_pool<nghttp2::server> server;
awaitable<int> echo(nghttp2::session &ctx, nghttp2::request &request)
{
    LOG_DBG("recv data path %s request %s", request.path().c_str(), request.body().c_str());

    nghttp2::header ngh;
    ngh.push_back(":status", "200");

    auto err = co_await ctx.reply(request.id(), ngh, request.body().c_str(), request.body().size());
    if (err == INVALID)
    {
        LOG_ERR("send error %d %s", errno, strerror(errno));
        co_return INVALID;
    }
    LOG_DBG("stream_id:%d send data %s %ld", request.id(), request.body().c_str(), request.body().size());
    co_return 0;
};
awaitable<void> proc_server()
{
    LOG_DBG("server start %s", "0.0.0.0:9999");
    auto &srv = server.get();
    srv.set_router("/echo", echo);
    co_await srv.dispatch(g_srv_mgr.get());
    co_return;
}
awaitable<void> proc_client()
{
    co_task _task;
    for (auto w = 0; w < coroutine_num; w++)
    {
        _task << []() -> awaitable<void>
        {
            for (auto i = 0; i < 1000000; i++)
            {
                coev::pool::nghttp2::Http2::instance c;
                auto r = co_await http2.get(c);
                if (r == INVALID)
                {
                    LOG_ERR("get http2 failed");
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
                    co_return;
                }
                LOG_DBG("status: %s body:%s", res.header(":status").c_str(), res.body().c_str());
            }
        }();
    }

    co_await _task.wait_all();
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

    if (strcmp(argv[1], "server") == 0)
    {
        server.start("0.0.0.0", 9999);
        g_srv_mgr.use_certificate_file("./certs/server/server.crt");
        g_srv_mgr.use_private_key_file("./certs/server/server.key");
        runnable::instance()
            .start(worker_num, proc_server)
            .end([]()
                 { server.stop(); });
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        auto config = http2.get_config();
        config->host = "0.0.0.0";
        config->port = 9999;
        config->data = g_cli_mgr.get();
        config->max_connections = max_connection;
        http2.set(config);
        runnable::instance()
            .start(worker_num, proc_client)
            .end([]()
                 { http2.stop(); });
    }

    return 0;
}