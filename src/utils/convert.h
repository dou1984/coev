#pragma once
#include <string>

inline std::string to_hex(const std::string &data)
{
    static const char *LOG_HEX = "0123456789abcdef";
    std::string res;
    res.reserve(data.size() * 2);
    for (unsigned char c : data)
    {
        res += LOG_HEX[c >> 4];
        res += LOG_HEX[c & 0xf];
    }
    return res;
}