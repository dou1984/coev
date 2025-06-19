#pragma once

namespace coev
{
    template <typename T>
    T CV(T &a)
    {
        T t = a;
        a = nullptr;
        return t;
    }
}