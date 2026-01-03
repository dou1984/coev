#pragma once
#include <cstring>
#include <coev/coev.h>

namespace coev
{
    awaitable<int> parse_dns(const std::string &url, std::string &addr);
}