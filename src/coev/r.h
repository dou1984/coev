#pragma once

namespace coev
{
    template <typename T>
    T R(T &a)
    {
        T t = a;
        a = nullptr;
        return t;
    }
}