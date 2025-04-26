#include <coev/coev.h>
#include <co_nghttp/NgServer.h>
#include <co_nghttp/NgSession.h>
#include <co_nghttp/NgClient.h>
#include <co_nghttp/NgRequest.h>

using namespace coev::nghttp2;
using namespace coev;
ssl::ssl_manager g_cli_mgr(ssl::ssl_manager::TLS_CLIENT);
ssl::ssl_manager g_srv_mgr(ssl::ssl_manager::TLS_SERVER);

char hi[] = R"(helloworld)";

awaitable<int> echo(NgSession &ctx, NgRequest &request)
{
    NgHeader ngh;
    ngh.push_back(":status", "200");
    ngh.push_back("server:", "coev");
    ngh.push_back("set-cookie:", "COEV=0000000000000000");
    ngh.push_back("keep-alive:", "timeout=15, max=100");

    const char data[] = "hi, everyone!";
    auto err = co_await ctx.submit_response(request.stream_id, ngh, ngh.size(), data, strlen(data) + 1);
    if (err == INVALID)
    {
        LOG_ERR("send error %d %s\n", errno, strerror(errno));
        co_return INVALID;
    }
    LOG_DBG("send data %s %ld\n", hi, strlen(hi) + 1);
    co_return 0;
};
awaitable<void> proc_server()
{
    LOG_DBG("server start %s\n", "0.0.0.0:8090");
    coev::nghttp2::NgServer server("0.0.0.0:8090");

    server.route("/echo", echo);
    co_await server.dispatch(g_srv_mgr.get());
    co_return;
}
awaitable<void> proc_client()
{
    coev::nghttp2::NgClient client(g_cli_mgr.get());
    int fd = co_await client.connect("0.0.0.0:8090");
    if (fd == INVALID)
    {
        co_return;
    }

    NgHeader ngh;
    ngh.push_back(":method", "GET");
    ngh.push_back(":path", "/");
    ngh.push_back(":scheme", "https");
    ngh.push_back("accept", "*/*");
    ngh.push_back("user-agent", "nghttp2/" NGHTTP2_VERSION);

    const char *user_data = "hello world";
    int length = strlen(user_data) + 1;

    int r = co_await client.submit_request(ngh, ngh.size());
    if (r == INVALID)
    {
        LOG_ERR("send error %d %s\n", errno, strerror(errno));
        co_return;
    }
    co_await client.processing();
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