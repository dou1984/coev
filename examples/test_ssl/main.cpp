#include <coev/coev.h>
#include <coev_ssl/ssl.h>

using namespace coev;
using namespace coev::ssl;
static manager g_srv_mgr(manager::TLS_SERVER);
static manager g_cli_mgr(manager::TLS_CLIENT);

awaitable<void> test_ssl_context()
{
    server_pool<tcp::server> pool;
    pool.start("0.0.0.0", 9998);
    g_srv_mgr.load_certificated("./server.pem");
    g_srv_mgr.load_privatekey("./server.pem");

    LOG_DBG("server started\n");
    while (true)
    {
        addrInfo addr;
        auto fd = co_await pool.get().accept(addr);
        LOG_DBG("accepted %d from %s:%d\n", fd, addr.ip, addr.port);

        if (fd == INVALID)
        {
            LOG_ERR("accept failed fd:%d\n", fd);
            continue;
        }
        co_start << [=]() -> awaitable<void>
        {
            ssl_context ctx(fd, g_srv_mgr.get());
            LOG_DBG("close fd:%d\n", fd);
            int err = co_await ctx.do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake failed fd:%d\n", fd);
                co_return;
            }
            while (true)
            {
                char buffer[1024];
                int r = co_await ctx.recv(buffer, sizeof(buffer));
                if (r == INVALID)
                {
                    LOG_ERR("recv failed fd:%d\n", fd);
                    co_return;
                }
                LOG_DBG("recv %d bytes from fd:%d %s\n", r, fd, buffer);
                r = co_await ctx.send("hello world", strlen("hello world") + 1);
                if (r == INVALID)
                {
                    LOG_ERR("send failed fd:%d\n", fd);
                    co_return;
                }
            }
        }();
    }
}
awaitable<void> test_ssl_client()
{

    ssl::ssl_client cli(g_cli_mgr.get());
    int fd = co_await cli.connect("0.0.0.0", 9998);
    if (fd == INVALID)
    {
        LOG_ERR("connect failed fd:%d error:%d\n", fd, errno);
        exit(INVALID);
    }
    LOG_DBG("connect success fd:%d\n", fd);
    for (int i = 0; i < 10; i++)
    {
        auto buf = "hello world";
        int size = strlen(buf) + 1;
        int r = co_await cli.send(buf, size);
        if (r == INVALID)
        {
            LOG_ERR("send failed fd:%d\n", fd);
            exit(INVALID);
        }
        char buffer[1024];
        r = co_await cli.recv(buffer, sizeof(buffer));
        if (r == INVALID)
        {
            LOG_ERR("recv failed fd:%d\n", fd);
            exit(INVALID);
        }
        LOG_DBG("recv %d bytes from %d %s\n", r, fd, buffer);
    }
}
int main(int argc, char **argv)
{
    set_log_level(LOG_LEVEL_DEBUG);
    if (argc < 2)
    {
        LOG_ERR("usage: %s [server|client]\n", argv[0]);
        exit(INVALID);
    }
    if (strcmp(argv[1], "server") == 0)
    {
        runnable::instance()
            .start(test_ssl_context)
            .join();
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        runnable::instance()
            .start(test_ssl_client)
            .join();
    }
    else
    {
        LOG_ERR("usage: %s [server|client]\n", argv[0]);
    }

    return 0;
}