#include <cstring>
#include <cassert>
#include <algorithm>
#include "length_field.h"

int LengthField::decode(packet_decoder &pd)
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
int LengthField::run(int cur_offset, std::string &buf)
{
    uint32_t val = static_cast<uint32_t>(cur_offset - m_start_offset - 4);
    buf[m_start_offset + 0] = (val >> 24) & 0xFF;
    buf[m_start_offset + 1] = (val >> 16) & 0xFF;
    buf[m_start_offset + 2] = (val >> 8) & 0xFF;
    buf[m_start_offset + 3] = val & 0xFF;
    return 0;
}

int LengthField::check(int cur_offset, const std::string_view & /*buf*/)
{
    if (static_cast<int32_t>(cur_offset - m_start_offset - 4) != m_length)
    {
        return ErrPacketDecodingError;
    }
    return 0;
}
