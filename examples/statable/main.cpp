#include <coev/coev.h>

using namespace coev;

statable<int> __test()
{
    LOG_DBG("__test 0\n");
    co_yield 0;
    LOG_DBG("__test 1\n");
    co_yield 1;
    LOG_DBG("__test 2\n");
    co_yield 2;
    LOG_DBG("__test 3\n");
    co_yield 3;
    LOG_DBG("__test 4\n");
}
awaitable<void> test()
{
    auto a = __test();
    while (!a)
    {
        std::cout << a() << std::endl;
    }
    co_return;
}
int main()
{
    set_log_level(LOG_LEVEL_DEBUG);
    runnable::instance().start(test).wait();
    return 0;
}