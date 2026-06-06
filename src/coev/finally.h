/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <functional>

class Finally final
{
public:
    Finally(std::function<void()> f) : m_finally(f) {}
    ~Finally() { m_finally(); }
    Finally(const Finally &) = delete;
    Finally &operator=(const Finally &) = delete;
    Finally(Finally &&) = delete;
    Finally &operator=(Finally &&) = delete;

private:
    std::function<void()> m_finally;
};

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define finally(body) Finally CONCAT(_finally_, __LINE__)([&]() { body; })
// #define finally Finally CONCAT(_finally_, __LINE__) = [&]()