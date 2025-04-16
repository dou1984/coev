#pragma once
#include <coroutine>
namespace coev
{
    template <typename T>    
    T exchangeX(T &org)
    {
        auto tmp = org;
        org = nullptr;
        return tmp;
    }
}