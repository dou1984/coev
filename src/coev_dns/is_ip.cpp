/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#include <regex>
#include "is_ip.h"

namespace coev
{
    static std::regex ipv4_re("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}$");
    static std::regex ipv6_re("^(\\[)?([0-9a-fA-F:]+)(\\])?$|:");
    bool is_ipv4(std::string_view view)
    {
        return std::regex_match(view.data(), ipv4_re);
    }
    bool is_ipv6(std::string_view view)
    {
        return std::regex_match(view.data(), ipv6_re);
    }
}