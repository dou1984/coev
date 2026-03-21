/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <cstdlib>
#include <new>
#include "queue.h"

#define CONTAINER_OF(TYPE, PTR, MEMBER) \
    reinterpret_cast<TYPE *>(reinterpret_cast<char *>(PTR) - ((TYPE *)(0))->MEMBER)

namespace coev
{
    namespace details
    {

        struct node : queue
        {
            int size;
            char data[0];
        };

        inline node *create(int size)
        {
            auto ptr = static_cast<node *>(std::malloc(size + sizeof(node)));
            if (ptr != nullptr)
            {
                ptr->size = size;
                return ptr;
            }
            throw std::bad_alloc();
        }

        inline void release(node *ptr)
        {
            std::free(ptr);
        }

    }

    template <class T, int... M>
    struct memory_pool;

    template <class T, int M>
    struct memory_pool<T, M> : queue
    {
        T *create(int size)
        {
            if (size <= M)
            {
                auto ptr = static_cast<details::node *>(pop_front());
                if (ptr != nullptr)
                {
                    return ptr->data;
                }
                ptr = details::create(M);
                return ptr->data;
            }
            auto ptr = details::create(size);
            return ptr->data;
        }
        void release(T *ptr)
        {
            if (ptr != nullptr)
            {
                auto q = CONTAINER_OF(details::node, ptr, data);
                __release(q);
            }
        }
        void __release(details::node *q)
        {
            assert(q != nullptr);
            if (q->size == M)
            {
                push_back(q);
            }
            else
            {
                free(q);
            }
        }
    };
    template <class T, int M, int... N>
    struct memory_pool<T, M, N...> : queue
    {
        memory_pool<T, N...> m_next;
        T *create(int size)
        {
            if (size <= M)
            {
                auto ptr = static_cast<details::node *>(pop_front());
                if (ptr != nullptr)
                {
                    return ptr->data;
                }
                ptr = details::create(M);
                return ptr->data;
            }
            return m_next.create(size);
        }
        void release(T *ptr)
        {
            if (ptr != nullptr)
            {
                auto q = CONTAINER_OF(details::node, ptr, data);
                __release(q);
            }
        }
        void __release(details::node *q)
        {
            if (q->size == M)
            {
                push_back(q);
            }
            else
            {
                m_next.__release(q);
            }
        }
    };
}