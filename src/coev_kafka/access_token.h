/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <unordered_map>
#include <map>

struct AccessToken
{
    std::string m_token;
    std::map<std::string, std::string> m_extensions;
};