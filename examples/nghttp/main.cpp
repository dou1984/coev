/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_nghttp/co_nghttp2.h>

using namespace coev;
ssl::manager g_cli_mgr(ssl::manager::TLS_CLIENT);
ssl::manager g_srv_mgr(ssl::manager::TLS_SERVER);

char hi[] = R"(helloworld)";

awaitable<int> echo(nghttp2::session &ctx, nghttp2::request &request)
{
    LOG_DBG("recv data path %s request %s", request.path().c_str(), request.body().c_str());

    nghttp2::header ngh;
    ngh.push_back(":status", "200");

    const char data[] = "hi, everyone!";
    auto err = ctx.reply(request.id(), ngh, data, strlen(data) + 1);
    if (err == INVALID)
    {
        LOG_ERR("send error %d %s", errno, strerror(errno));
        co_return INVALID;
    }
    LOG_DBG("send data %s %ld", data, strlen(data) + 1);
    co_return 0;
};
awaitable<void> proc_server()
{
    LOG_DBG("server start %s", "0.0.0.0:8090");
    nghttp2::server server("0.0.0.0:8090");

    server.route("/echo", echo);
    co_await server.dispatch(g_srv_mgr.get());
    co_return;
}
awaitable<void> proc_client()
{
    nghttp2::client cli(g_cli_mgr.get());
    int fd = co_await cli.connect("0.0.0.0:8090");
    if (fd == INVALID)
    {
        co_return;
    }

    co_start << cli.processing();

    nghttp2::header ngh;
    ngh.push_back(":method", "GET");
    ngh.push_back(":path", "/echo");
    ngh.push_back(":scheme", "https");
    ngh.push_back(":authority", "coev");
    ngh.push_back("accept", "*/*");
    ngh.push_back("user-agent", "nghttp2/" NGHTTP2_VERSION);

    auto res = co_await cli.query(ngh, hi, sizeof(hi));

    LOG_INFO("status: %s body:%s", res.header(":status").c_str(), res.body().c_str());

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

    if (strcmp(argv[1], "server") == 0)
    {
        g_srv_mgr.use_certificate_file("server.pem");
        g_srv_mgr.use_private_key_file("server.pem");
        runnable::instance()
            .start(proc_server)
            .wait();
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        runnable::instance()
            .start(proc_client)
            .wait();
    }

    return 0;
}