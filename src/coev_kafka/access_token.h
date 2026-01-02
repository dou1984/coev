#pragma once
#include <string>
#include <unordered_map>
#include <map>

struct AccessToken
{
    std::string Token;
    std::map<std::string, std::string> Extensions;
};