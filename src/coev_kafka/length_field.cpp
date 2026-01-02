#include <cstring>
#include <algorithm>
#include "length_field.h"

int LengthField::decode(PDecoder &pd)
{
    int err = pd.getInt32(length);
    if (err != 0)
        return err;
    if (length > static_cast<int32_t>(pd.remaining()))
        return ErrInsufficientData;
    return 0;
}

void LengthField::saveOffset(int in)
{
    startOffset = in;
}

int LengthField::reserveLength()
{
    return 4;
}
int LengthField::run(int curOffset, std::string &buf)
{
    uint32_t val = static_cast<uint32_t>(curOffset - startOffset - 4);
    buf[startOffset + 0] = (val >> 24) & 0xFF;
    buf[startOffset + 1] = (val >> 16) & 0xFF;
    buf[startOffset + 2] = (val >> 8) & 0xFF;
    buf[startOffset + 3] = val & 0xFF;
    return 0;
}

int LengthField::check(int curOffset, const std::string & /*buf*/)
{
    if (static_cast<int32_t>(curOffset - startOffset - 4) != length)
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
    if (ptr != nullptr)
        return;
    ptr->startOffset = 0;
    ptr->length = 0;
    std::lock_guard<std::mutex> lock(g_lengthFieldPoolMutex);
    g_lengthFieldPool.push(ptr);
}