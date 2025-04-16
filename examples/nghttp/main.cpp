#include <coev/coev.h>
#include <co_nghttp/NghttpServer.h>
#include <co_nghttp/NghttpSession.h>
#include <co_nghttp/NghttpConnect.h>

using namespace coev;

ssl_manager g_cli_mgr(ssl_manager::TLS_CLIENT);
ssl_manager g_srv_mgr(ssl_manager::TLS_SERVER);

char hi[] = R"(helloworld)";

const auto echo = [](coev::nghttp2::NghttpSession &ctx) -> awaitable<void>
{
    NgHeader ngh;
    ngh.push_back(":status", "200");
    ngh.push_back("server:", "coev");
    ngh.push_back("set-cookie:", "COEV=0000000000000000");
    ngh.push_back("keep-alive:", "timeout=5, max=100");
    ngh.push_back("content-length:", "11");

    auto err = co_await ctx.send_body(ngh, ngh.size(), hi, strlen(hi) + 1);
    if (err == INVALID)
    {
        LOG_ERR("send error %d %s\n", errno, strerror(errno));
        co_return;
    }
    LOG_DBG("send data %s %ld\n", hi, strlen(hi) + 1);
    co_return;
};
awaitable<void> proc_server()
{
    LOG_DBG("server start %s\n", "0.0.0.0:8090");
    coev::nghttp2::NghttpServer server("0.0.0.0:8090");

    while (server.valid())
    {
        addrInfo info;
        int fd = co_await server.accept(info);

        LOG_DBG("recv fd %d from %s:%d\n", fd, info.ip, info.port);
        co_start << [=]() -> awaitable<int>
        {
            LOG_DBG("client start %d\n", fd);
            coev::nghttp2::NghttpSession ctx(fd, g_srv_mgr.get());
            auto err = co_await ctx.do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake error %d %s\n", errno, strerror(errno));
                co_return INVALID;
            }
            co_await ctx.process_loop();
            co_return 0;
        }();
    }
    co_return;
}
awaitable<void> proc_client()
{
    coev::nghttp2::NghttpConnect client(g_cli_mgr.get());
    int fd = co_await client.connect("0.0.0.0:8090");
    if (fd == INVALID)
    {
        co_return;
    }

    NgHeader ngh;
    ngh.push_back(":method", "GET");
    ngh.push_back(":scheme", "https");
    ngh.push_back("accept", "*/*");
    ngh.push_back("user-agent", "nghttp2/" NGHTTP2_VERSION);

    const char *user_data = "hello world";
    int length = strlen(user_data) + 1;

    int r = co_await client.send_body(ngh, ngh.size(), user_data, length);
    if (r == INVALID)
    {
        LOG_ERR("send error %d %s\n", errno, strerror(errno));
        co_return;
    }
    co_await client.process_loop();
    co_return;
}
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LOG_ERR("usage: %s server|client\n", argv[0]);
        return 1;
    }

    set_log_level(LOG_LEVEL_CORE);

    if (strcmp(argv[1], "server") == 0)
    {
        g_srv_mgr.load_certificated("server.pem");
        g_srv_mgr.load_privatekey("server.pem");
        runnable::instance()
            .start(proc_server)
            .join();
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        runnable::instance()
            .start(proc_client)
            .join();
    }

    return 0;
}