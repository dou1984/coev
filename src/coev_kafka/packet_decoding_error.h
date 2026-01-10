#pragma once
#include <string>

struct PacketDecodingError
{
    std::string m_message;
    PacketDecodingError(const std::string &msg) : m_message(msg) {}
};