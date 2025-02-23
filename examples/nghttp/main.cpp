#include <coev/coev.h>
#include <co_nghttp/NghttpServer.h>
#include <co_nghttp/NghttpRequest.h>

using namespace coev;
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
            io_context ctx(fd);

            char buffer[1000];
            co_await ctx.recv(buffer, sizeof(buffer));
           
            co_await ctx.send(hi, strlen(hi) + 1);
        }();
    }
    co_return 0;
}
awaitable<int> proc_client()
{
    coev::nghttp2::NghttpRequest client;
    int fd = co_await client.connect("127.0.0.1");
    if (fd == INVALID)
    {
        co_return INVALID;
    }
    const char *user_data = "GET / HTTP/1.1\r\nHost: 157.148.69.80\r\n\r\n";
    int length = strlen(user_data) + 1;
    co_await client.send(user_data, length);

    co_return 0;
}
int main()
{
    set_log_level(LOG_LEVEL_DEBUG);
    running::instance()
        .add(proc_server)
        // .add(proc_client)
        .join();
    return 0;
}