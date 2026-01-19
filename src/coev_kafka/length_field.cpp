#include <cstring>
#include <algorithm>
#include "length_field.h"

int LengthField::decode(packetDecoder &pd)
{
    int err = pd.getInt32(m_length);
    if (err != 0)
        return err;
    if (m_length > static_cast<int32_t>(pd.remaining()))
        return ErrInsufficientData;
    return 0;
}

void LengthField::save_offset(int in)
{
    m_start_offset = in;
}

int LengthField::reserve_length()
{
    return 4;
}
int LengthField::run(int curOffset, std::string &buf)
{
    uint32_t val = static_cast<uint32_t>(curOffset - m_start_offset - 4);
    buf[m_start_offset + 0] = (val >> 24) & 0xFF;
    buf[m_start_offset + 1] = (val >> 16) & 0xFF;
    buf[m_start_offset + 2] = (val >> 8) & 0xFF;
    buf[m_start_offset + 3] = val & 0xFF;
    return 0;
}

int LengthField::check(int curOffset, const std::string & /*buf*/)
{
    if (static_cast<int32_t>(curOffset - m_start_offset - 4) != m_length)
    {
        return ErrPacketDecodingError;
    }
    return 0;
}

std::mutex g_lengthFieldPoolMutex;
std::stack<std::shared_ptr<LengthField>> g_lengthFieldPool;

std::shared_ptr<LengthField> acquireLengthField()
{
    std::lock_guard<std::mutex> lock(g_lengthFieldPoolMutex);
    if (!g_lengthFieldPool.empty())
    {
        auto ptr = g_lengthFieldPool.top();
        g_lengthFieldPool.pop();
        return ptr;
    }
    return std::make_shared<LengthField>();
}

void releaseLengthField(std::shared_ptr<LengthField> ptr)
{
    if (ptr == nullptr)
        return;
    ptr->m_start_offset = 0;
    ptr->m_length = 0;
    std::lock_guard<std::mutex> lock(g_lengthFieldPoolMutex);
    g_lengthFieldPool.push(ptr);
}