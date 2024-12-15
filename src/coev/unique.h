#pragma once
#include <unordered_map>
#include <mutex>
#include "gtid.h"
namespace coev
{
    template <class T>
    class unique
    {
    public:
        unique()
        {
            m_tid = gtid();
            std::lock_guard<std::mutex> _(m_mutex);
            all_datas.emplace(m_tid, &m_data);
        }
        ~unique()
        {
            std::lock_guard<std::mutex> _(m_mutex);
            all_datas.erase(m_tid);
        }
        T *operator->() { return &m_data; }
        static T *at(size_t id)
        {
            std::lock_guard<std::mutex> _(m_mutex);
            auto it = all_datas.begin();
            return it != all_datas.end() ? it->second : nullptr;
        }
        template <class Func>
        static void foreach (const Func &func)
        {
            std::lock_guard<std::mutex> _(m_mutex);
            for (auto &it : all_datas)
            {
                func(it);
            }
        }

    private:
        static std::unordered_map<uint64_t, T *> all_datas;
        static std::mutex m_mutex;
        uint64_t m_tid = 0;
        T m_data;
    };
    template <class T>
    std::unordered_map<uint64_t, T *> unique<T>::all_datas;
    template <class T>
    std::mutex unique<T>::m_mutex;
}