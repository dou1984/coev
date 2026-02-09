#pragma once
#include <string>
#include <stdexcept>

struct PacketDecodingError : public std::exception
{
    std::string m_message;
    PacketDecodingError(const std::string &msg) : m_message(msg) {}
    const char* what() const noexcept override {
        return m_message.c_str();
    }
};