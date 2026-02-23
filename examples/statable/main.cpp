/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <coev/coev.h>

using namespace coev;

statable<int> __test()
{
    LOG_DBG("__test 0");
    co_yield 0;
    LOG_DBG("__test 1");
    co_yield 1;
    LOG_DBG("__test 2");
    co_yield 2;
    LOG_DBG("__test 3");
    co_yield 3;
    LOG_DBG("__test 4");
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