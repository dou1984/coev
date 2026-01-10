#pragma once
#include <string>
#include <unordered_map>
#include <map>

struct AccessToken
{
    std::string m_token;
    std::map<std::string, std::string> m_extensions;
};