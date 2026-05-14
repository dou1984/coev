/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <functional>

namespace coev
{
    class Finally final
    {
    public:
        Finally(std::function<void()> f) : m_finally(f) {}
        ~Finally() { m_finally(); }

        // 禁止拷贝和移动
        Finally(const Finally &) = delete;
        Finally &operator=(const Finally &) = delete;
        Finally(Finally &&) = delete;
        Finally &operator=(Finally &&) = delete;

    private:
        std::function<void()> m_finally;
    };

}
// 宏定义简化使用
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define defer(body) coev::Finally CONCAT(_defer_, __LINE__)([&]() { body; })