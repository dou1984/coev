/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include <coev/coev.h>
#include <coev_zookeeper/Zoo.h>

using namespace coev;

coev::pool::Zoo zoo;

int main(int argc, char **argv)
{
    auto config = zoo.get_config();
    config->host = "127.0.0.1";
    config->port = 2181;
    zoo.set(config);

    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                coev::pool::Zoo::instance c;
                auto err = co_await zoo.get(c);
                if (err == INVALID)
                {
                    LOG_ERR("get error %d", err);
                    co_return;
                }

                err = co_await c->set("/test", "hello", -1);
                if (err == INVALID)
                {
                    co_return;
                }
                std::string body;
                err = co_await c->get("/test", body);
                if (err == INVALID)
                {
                    co_return;
                }

                LOG_DBG("%s", body.c_str());
                co_await sleep_for(1000);

                co_return;
            })
        .end([]()
                 { zoo.stop(); });

    return 0;
}