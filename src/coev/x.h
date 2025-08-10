#pragma once

namespace coev
{
    template <typename T>
    T X(T &a)
    {
        T t = a;
        a = nullptr;
        return t;
    }
}