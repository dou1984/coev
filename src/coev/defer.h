#pragma once

#include <functional>

namespace coev
{
    template <class F, class... Args>
    class defer
    {
    public:
        defer(const F &f, const Args &...args)
        {
            __f = [&]()
            { f(args...); };
        }
        ~defer()
        {
            __f();
        }

    private:
        std::function<void()> __f;
    };
}