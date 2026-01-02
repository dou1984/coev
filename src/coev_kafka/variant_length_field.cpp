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

int VariantLengthField::decode(PDecoder &pd)
{
    int64_t val;
    int err = pd.getVariant(val);
    if (err != 0)
        return err;
    length = val;
    return 0;
}

void VariantLengthField::saveOffset(int in)
{
    startOffset = in;
}

int VariantLengthField::reserveLength()
{
    return variantSize(length);
}

int VariantLengthField::adjustLength(int currOffset)
{
    int oldSize = reserveLength();
    length = static_cast<int64_t>(currOffset - startOffset - oldSize);
    int newSize = reserveLength();
    return newSize - oldSize;
}

int VariantLengthField::run(int curOffset, std::string &buf)
{
    int n = encodeVariant((uint8_t *)buf.data() + startOffset, length);
    return (n > 0) ? 0 : -1;
}

int VariantLengthField::check(int curOffset, const std::string & /*buf*/)
{
    if (static_cast<int64_t>(curOffset - startOffset - reserveLength()) != length)
    {
        return -1;
    }
    return 0;
}
