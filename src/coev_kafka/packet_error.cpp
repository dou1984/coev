/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#include "packet_error.h"

namespace coev::kafka
{

    PacketError::PacketError(const std::string &msg) : m_message(msg)
    {
    }
    const char *PacketError::what() const noexcept
    {
        return m_message.c_str();
    }

} // namespace coev::kafka
