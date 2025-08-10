#include <coev/coev.h>
#include <coev_zookeeper/ZooCli.h>

using namespace coev;

struct zoo_config
{
};

zoo_config g_config;

int main(int argc, char **argv)
{

    runnable::instance()
        .start(
            []() -> awaitable<void>
            {
                ZooCli cli;

                auto r = co_await cli.connect("127.0.0.1", 2181);
                if (r == INVALID)
                {
                    co_return;
                }

                r = co_await cli.zoo_set("/test", "hello", -1);
                if (r == INVALID)
                {
                    co_return;
                }
                std::string body;
                r = co_await cli.zoo_get("/test", body);
                if (r == INVALID)
                {
                    co_return;
                }

                LOG_DBG("%s\n", body.c_str());
                co_await sleep_for(1000);

                co_return;
            })
        .join();

    return 0;
}