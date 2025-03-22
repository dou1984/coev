#include <coev/coev.h>
#include <coev/cosys/cosys.h>
#include <co_nghttp/NghttpServer.h>
#include <co_nghttp/NghttpRequest.h>

using namespace coev;

ssl_manager g_cli_mgr(ssl_manager::TLS_CLIENT);

awaitable<int> proc_server()
{
    LOG_DBG("server start %s", "0.0.0.0:8090");
    coev::nghttp2::NghttpServer server("0.0.0.0:8090");
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
    while (server.valid())
    {
        addrInfo info;
        int fd = co_await server.accept(info);

        LOG_DBG("recv fd %d from %s:%d", fd, info.ip, info.port);
        [=]() -> awaitable<void>
        {
            coev::nghttp2::NghttpRequest ctx(fd, g_cli_mgr.get());

            char buffer[1000];
            auto err = co_await ctx.recv_body(buffer, sizeof(buffer));
            if (err == INVALID)
            {
                LOG_ERR("recv error %d %s\n", errno, strerror(errno));
                co_return;
            }
            LOG_DBG("recv data %s %d\n", buffer, err);
            err = co_await ctx.send_body(hi, strlen(hi) + 1);
            if (err == INVALID)
            {
                LOG_ERR("send error %d %s\n", errno, strerror(errno));
                co_return;
            }
            LOG_DBG("send data %s %ld\n", hi, strlen(hi) + 1);
        }();
    }
    co_return 0;
}
awaitable<int> proc_client()
{
    coev::nghttp2::NghttpRequest client;
    int fd = co_await client.connect("127.0.0.1:8090");
    if (fd == INVALID)
    {
        co_return INVALID;
    }
    const char *user_data = "GET / HTTP/1.1\r\nHost: 157.148.69.80\r\n\r\n";
    int length = strlen(user_data) + 1;
    int err = co_await client.send_body(user_data, length);
    char buffer[1000];
    err = co_await client.recv_body(buffer, sizeof(buffer));
    co_return 0;
}
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        LOG_ERR("usage: %s server|client", argv[0]);
        return 1;
    }

    set_log_level(LOG_LEVEL_DEBUG);

    if (strcmp(argv[1], "server") == 0)
    {
        running::instance()
            .add(proc_server)
            .join();
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        running::instance()
            .add(proc_client)
            .join();
    }

    return 0;
}