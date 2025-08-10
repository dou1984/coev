#pragma once
#include <functional>

namespace coev
{
    class Defer
    {
    public:
        Defer(std::function<void()> f) : deferred_function(f) {}
        ~Defer() { deferred_function(); }

        // 禁止拷贝和移动
        Defer(const Defer &) = delete;
        Defer &operator=(const Defer &) = delete;
        Defer(Defer &&) = delete;
        Defer &operator=(Defer &&) = delete;

    private:
        std::function<void()> deferred_function;
    };

}
// 宏定义简化使用
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define defer(body) Defer CONCAT(_defer_, __LINE__)([&]() { body; })