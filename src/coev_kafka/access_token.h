/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <unordered_map>
#include <map>

namespace coev::kafka
{

    struct AccessToken
    {
        std::string m_token;
        std::map<std::string, std::string> m_extensions;
    };

} // namespace coev::kafka