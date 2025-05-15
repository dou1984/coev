#include <coev/coev.h>
#include <co_nghttp/ng_server.h>
#include <co_nghttp/ng_session.h>
#include <co_nghttp/ng_client.h>
#include <co_nghttp/ng_request.h>

using namespace coev::nghttp2;
using namespace coev;
ssl::ssl_manager g_cli_mgr(ssl::ssl_manager::TLS_CLIENT);
ssl::ssl_manager g_srv_mgr(ssl::ssl_manager::TLS_SERVER);

char hi[] = R"(helloworld)";

awaitable<int> echo(ng_session &ctx, ng_request &request)
{
    LOG_DBG("recv data path %s request %s\n", request.path().c_str(), request.body().c_str());

    ng_header ngh;
    ngh.push_back(":status", "200");

    const char data[] = "hi, everyone!";
    auto err = ctx.submit_response(request.id(), ngh, ngh.size(), data, strlen(data) + 1);
    if (err == INVALID)
    {
        LOG_ERR("send error %d %s\n", errno, strerror(errno));
        co_return INVALID;
    }
    LOG_DBG("send data %s %ld\n", data, strlen(data) + 1);
    co_return 0;
};
awaitable<void> proc_server()
{
    LOG_DBG("server start %s\n", "0.0.0.0:8090");
    coev::nghttp2::ng_server server("0.0.0.0:8090");

    server.route("/echo", echo);
    co_await server.dispatch(g_srv_mgr.get());
    co_return;
}
awaitable<void> proc_client()
{
    coev::nghttp2::ng_client cli(g_cli_mgr.get());
    int fd = co_await cli.connect("0.0.0.0:8090");
    if (fd == INVALID)
    {
        co_return;
    }

    co_start << cli.processing();

    ng_header ngh;
    ngh.push_back(":method", "GET");
    ngh.push_back(":path", "/echo");
    ngh.push_back(":scheme", "https");
    ngh.push_back(":authority", "coev");
    ngh.push_back("accept", "*/*");
    ngh.push_back("user-agent", "nghttp2/" NGHTTP2_VERSION);

    int32_t stream_id = cli.submit_request(ngh, ngh.size(), hi, sizeof(hi));
    if (stream_id == INVALID)
    {
        LOG_ERR("send error %d %s\n", errno, strerror(errno));
        co_return;
    }

    auto res = co_await cli.wait_for_stream_end(stream_id);

    LOG_INFO("status: %s body:%s\n", res.header(":status").c_str(), res.body().c_str());

    
    cli.close();
    co_return;
}
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LOG_ERR("usage: %s server|ssl_client\n", argv[0]);
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