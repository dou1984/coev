#include "variant_length_field.h"
#include "real_encoder.h"
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

int VariantLengthField::decode(packetDecoder &pd)
{
    int64_t val;
    int err = pd.getVariant(val);
    if (err != 0)
        return err;
    m_length = val;
    return 0;
}

void VariantLengthField::saveOffset(int in)
{
    m_start_offset = in;
}

int VariantLengthField::reserveLength()
{
    return variantSize(m_length);
}

int VariantLengthField::adjust_length(int currOffset)
{
    int oldSize = reserveLength();
    m_length = static_cast<int64_t>(currOffset - m_start_offset - oldSize);
    int newSize = reserveLength();
    return newSize - oldSize;
}

int VariantLengthField::run(int curOffset, std::string &buf)
{
    int n = encodeVariant((uint8_t *)buf.data() + m_start_offset, m_length);
    return (n > 0) ? 0 : -1;
}

int VariantLengthField::check(int curOffset, const std::string & /*buf*/)
{
    if (static_cast<int64_t>(curOffset - m_start_offset - reserveLength()) != m_length)
    {
        return -1;
    }
    return 0;
}
