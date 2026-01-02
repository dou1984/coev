#pragma once
#include <string>

struct PacketDecodingError
{
    std::string message;
    PacketDecodingError(const std::string &msg) : message(msg) {}
};