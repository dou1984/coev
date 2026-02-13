#include "packet_error.h"

PacketError::PacketError(const std::string &msg) : m_message(msg)
{
}
const char *PacketError::what() const noexcept
{
    return m_message.c_str();
}
