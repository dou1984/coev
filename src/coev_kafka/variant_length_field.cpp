#include "variant_length_field.h"
#include "real_encoder.h"

extern int encodeVariant(uint8_t *buf, int64_t x);
static int variantSize(int64_t value)
{
    int size = 0;
    do
    {
        value >>= 7;
        ++size;
    } while (value != 0);
    return size;
}

int VariantLengthField::decode(packet_decoder &pd)
{
    int64_t val;
    int err = pd.getVariant(val);
    if (err != 0)
    {
        return err;
    }
    m_length = val;
    return 0;
}

void VariantLengthField::save_offset(int in)
{
    m_start_offset = in;
}

int VariantLengthField::reserve_length()
{
    return variantSize(m_length);
}

int VariantLengthField::adjust_length(int currOffset)
{
    int oldSize = reserve_length();
    m_length = static_cast<int64_t>(currOffset - m_start_offset - oldSize);
    int newSize = reserve_length();
    return newSize - oldSize;
}

int VariantLengthField::run(int curOffset, std::string &buf)
{
    int n = encodeVariant((uint8_t *)buf.data() + m_start_offset, m_length);
    return (n > 0) ? 0 : -1;
}

int VariantLengthField::check(int curOffset, const std::string_view & /*buf*/)
{
    if (static_cast<int64_t>(curOffset - m_start_offset - reserve_length()) != m_length)
    {
        return -1;
    }
    return 0;
}
