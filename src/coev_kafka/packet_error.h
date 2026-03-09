/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <stdexcept>

namespace coev::kafka
{

    struct PacketError : std::exception
    {
        std::string m_message;
        PacketError(const std::string &msg);
        const char *what() const noexcept override;
    };

} // namespace coev::kafka
