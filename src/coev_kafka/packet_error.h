#pragma once
#include <string>
#include <stdexcept>

struct PacketError : std::exception
{
    std::string m_message;
    PacketError(const std::string &msg);
    const char *what() const noexcept override;
};
