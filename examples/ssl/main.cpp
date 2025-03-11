#include <cosys/cosys.h>
#include <coev/coev.h>

using namespace coev;

static ssl_manager g_srv_mgr(ssl_manager::TLS_SERVER);
static ssl_manager g_cli_mgr(ssl_manager::TLS_CLIENT);

awaitable<void> test_ssl_context()
{
    server_pool<tcp::server> pool;
    pool.start("127.0.0.1", 9998);
    g_srv_mgr.load_certificated(
        "./server.pem",
        "./server.pem");

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
        [=]() -> awaitable<void>
        {
            ssl_context ctx(fd, g_srv_mgr.get());
            int err = co_await ctx.do_handshake();
            if (err == INVALID)
            {
                LOG_ERR("handshake failed fd:%d\n", fd);
                co_return;
            }
            while (ctx)
            {
                char buffer[1024];
                int r = co_await ctx.recv(buffer, sizeof(buffer));
                if (r == INVALID)
                {
                    LOG_ERR("recv failed fd:%d\n", fd);
                    break;
                }
                r = co_await ctx.send("hello world", strlen("hello world") + 1);
                if (r == INVALID)
                {
                    LOG_ERR("send failed fd:%d\n", fd);
                    break;
                }
            }
        }();
    }
}
awaitable<void> test_ssl_client()
{

    ssl_context client(g_cli_mgr.get());
    int fd = co_await client.connect("127.0.0.1", 9998);
    if (fd == INVALID)
    {
        LOG_ERR("connect failed fd:%d\n", fd);
        exit(INVALID);
    }
    auto buf = "hello world";
    int size = strlen(buf) + 1;
    int r = co_await client.send(buf, size);
    if (r == INVALID)
    {
        LOG_ERR("send failed fd:%d\n", fd);
        exit(INVALID);
    }
    char buffer[1024];
    r = co_await client.recv(buffer, sizeof(buffer));
    if (r == INVALID)
    {
        LOG_ERR("recv failed fd:%d\n", fd);
        exit(INVALID);
    }
    LOG_DBG("recv %d bytes from %d\n", r, fd);
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
        running::instance()
            .add(test_ssl_context)
            .join();
    }
    else if (strcmp(argv[1], "client") == 0)
    {
        running::instance()
            .add(test_ssl_client)
            .join();
    }
    else
    {
        LOG_ERR("usage: %s [server|client]\n", argv[0]);
    }

    return 0;
}