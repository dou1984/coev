#pragma once
#include <hiredis/hiredis.h>
#include <coev/coev.h>

namespace coev
{
    class RedisArray
    {
        redisReply **m_element = nullptr;
        size_t m_elements;

    public:
        RedisArray(redisReply **element, size_t elements);
        bool operator==(const RedisArray &o) const { return m_element == o.m_element && m_elements == o.m_elements; }
        bool operator!=(const RedisArray &o) const { return m_element != o.m_element || m_elements != o.m_elements; }
        RedisArray begin() const { return RedisArray(m_element, m_elements); }
        RedisArray end() const { return RedisArray(m_element + m_elements, 0); }
        RedisArray shift(int i) const { return RedisArray(m_element + i, m_elements - i); }
        operator bool() const { return m_elements > 0; }
        template <class... ARGS>
        auto result(ARGS &&...args)
        {
            int i = 0;
            (__setvalue(args, m_element[i++]->str), ...);
            return shift(i);
        }
    };
}