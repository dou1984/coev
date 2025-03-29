#include <coev/coev.h>
#include <co_nghttp/NghttpServer.h>
#include <co_nghttp/NghttpSession.h>
#include <co_nghttp/NghttpConnect.h>

using namespace coev;

ssl_manager g_cli_mgr(ssl_manager::TLS_CLIENT);
ssl_manager g_srv_mgr(ssl_manager::TLS_SERVER);

char hi[] =
    R"(HTTP/1.1 200 OK
Date: Sun, 26 OCT 1984 10:00:00 GMT
Server: coev
Set-Cookie: COEV=0000000000000000; path=/
Expires: Sun, 26 OCT 2100 10:00:00 GMT
Cache-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0
Pragma: no-cache
Content-Length: 0
Keep-Alive: timeout=5, max=100
Connection: Keep-Alive
Content-Type: text/html; charset=utf-8)";

awaitable<void> proc_server()
{
    LOG_DBG("server start %s\n", "0.0.0.0:8090");
    coev::nghttp2::NghttpServer server("0.0.0.0:8090");

    while (server.valid())
    {
        addrInfo info;
        int fd = co_await server.accept(info);

        LOG_DBG("recv fd %d from %s:%d", fd, info.ip, info.port);
        co_start << [=]() -> awaitable<void>
        {
            coev::nghttp2::NghttpSession ctx(fd, g_srv_mgr.get());
            auto err = co_await ctx.do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake error %d %s\n", errno, strerror(errno));
                co_return;
            }

            char buffer[1000];
            err = co_await ctx.recv_body(buffer, sizeof(buffer));
            if (err == INVALID)
            {
                LOG_ERR("recv error %d %s\n", errno, strerror(errno));
                co_return;
            }
            LOG_DBG("recv data %s %d\n", buffer, err);
            Ngheader ngh;
            ngh.push_back(":status", "200");
            err = co_await ctx.send_body(ngh, ngh.size(), hi, strlen(hi) + 1);
            if (err == INVALID)
            {
                LOG_ERR("send error %d %s\n", errno, strerror(errno));
                co_return;
            }
            LOG_DBG("send data %s %ld\n", hi, strlen(hi) + 1);
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

    Ngheader ngh;
    ngh.push_back(":method", "GET");
    ngh.push_back(":scheme", "https");
    ngh.push_back("accept", "*/*");
    ngh.push_back("user-agent", "nghttp2/" NGHTTP2_VERSION);

    const char *user_data = "GET / HTTP/1.1\r\nHost: 157.148.69.80\r\n\r\n";
    int length = strlen(user_data) + 1;

    int err = co_await client.send_body(ngh, ngh.size(), user_data, length);
    char buffer[1000];

    err = co_await client.recv_body(buffer, sizeof(buffer));
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
            .add(proc_server)
            .join();
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        runnable::instance()
            .add(proc_client)
            .join();
    }

    return 0;
}