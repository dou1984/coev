#include "prep_encoder.h"
#include <stdexcept>
#include <cmath>
#include <limits>

static int varintLen(int64_t x)
{
    int s = 0;
    while (x >= 0x80 || x < -0x40)
    {
        x >>= 7;
        s++;
    }
    return s + 1;
}

static int uvarintLen(uint64_t x)
{
    if (x == 0)
        return 1;
    int s = 0;
    while (x >= 0x80)
    {
        x >>= 7;
        s++;
    }
    return s + 1;
}

prep_encoder::prep_encoder() = default;

void prep_encoder::putInt8(int8_t /*in*/)
{
    m_length += 1;
}
void prep_encoder::putInt16(int16_t /*in*/)
{
    m_length += 2;
}
void prep_encoder::putInt32(int32_t /*in*/)
{
    m_length += 4;
}
void prep_encoder::putInt64(int64_t /*in*/)
{
    m_length += 8;
}
void prep_encoder::putVariant(int64_t in)
{
    m_length += varintLen(in);
}
void prep_encoder::putUVarint(uint64_t in)
{
    m_length += uvarintLen(in);
}
void prep_encoder::putFloat64(double /*in*/)
{
    m_length += 8;
}

int prep_encoder::putArrayLength(int32_t in)
{
    if (_is_fixed())
    {
        if (in > std::numeric_limits<int32_t>::max())
        {
            return -1;
        }
        m_length += 4;
        return 0;
    }
    else if (_is_flexible())
    {
        putUVarint(static_cast<uint64_t>(in + 1));
        return 0;
    }
    return 0;
}
void prep_encoder::putBool(bool /*in*/)
{
    m_length++;
}
void prep_encoder::putKError(KError /*in*/)
{
    m_length += 2;
}
void prep_encoder::putDurationMs(std::chrono::milliseconds /*in*/)
{
    m_length += 4;
}

int prep_encoder::putBytes(const std::string_view &in)
{
    if (_is_fixed())
    {
        m_length += 4;
        if (in.empty())
            return 0;
        if (in.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        {
            return -1;
        }
        return putRawBytes(in);
    }
    else if (_is_flexible())
    {
        putUVarint(in.size() + 1);
        return putRawBytes(in);
    }
    return 0;
}

int prep_encoder::putVariantBytes(const std::string_view &in)
{
    if (in.empty())
    {
        putVariant(-1);
        return 0;
    }
    putVariant(static_cast<int64_t>(in.size()));
    return putRawBytes(in);
}

int prep_encoder::putRawBytes(const std::string_view &in)
{
    if (in.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
    {
        return -1;
    }
    m_length += static_cast<int>(in.size());
    return 0;
}

int prep_encoder::putString(const std::string_view &in)
{
    if (_is_fixed())
    {
        if (in.length() > static_cast<size_t>(std::numeric_limits<int16_t>::max()))
        {
            return -1;
        }
        m_length += 2;
        m_length += static_cast<int>(in.length());
        return 0;
    }
    else if (_is_flexible())
    {
        int err = putArrayLength(static_cast<int32_t>(in.length()));
        if (err != 0)
            return err;
        return putRawBytes(in);
    }
    return 0;
}

int prep_encoder::putNullableString(const std::string_view &in)
{
    if (_is_fixed())
    {
        if (in.empty())
        {
            m_length += 2; // null string encoded as -1 (int16)
            return 0;
        }
        return putString(in);
    }
    else if (_is_flexible())
    {
        if (in == "")
        {
            putUVarint(0);
            return 0;
        }
        return putString(in);
    }
    return 0;
}

int prep_encoder::putStringArray(const std::vector<std::string> &in)
{
    if (_is_fixed())
    {
        int err = putArrayLength(static_cast<int32_t>(in.size()));
        if (err != 0)
            return err;
        for (auto &str : in)
        {
            err = putString(str);
            if (err != 0)
                return err;
        }
        return 0;
    }
    else if (_is_flexible())
    {
        int err = putArrayLength(static_cast<int32_t>(in.size()));
        if (err != 0)
            return err;
        m_length += 4 * static_cast<int>(in.size());
        return 0;
    }
    return 0;
}

int prep_encoder::putNullableInt32Array(const std::vector<int32_t> &in)
{
    if (_is_fixed())
    {
        if (in.empty())
        {
            m_length += 4; // null array = -1 (int32)
            return 0;
        }
        int err = putArrayLength(static_cast<int32_t>(in.size()));
        if (err != 0)
            return err;
        m_length += 4 * static_cast<int>(in.size());
        return 0;
    }
    else if (_is_flexible())
    {
        if (in.empty())
        {
            putUVarint(0);
            return 0;
        }
        putUVarint(static_cast<uint64_t>(in.size()) + 1);
        m_length += 4 * static_cast<int>(in.size());
        return 0;
    }
    return 0;
}

int prep_encoder::putInt64Array(const std::vector<int64_t> &in)
{
    int err = putArrayLength(static_cast<int32_t>(in.size()));
    if (err != 0)
        return err;
    m_length += 8 * static_cast<int>(in.size());
    return 0;
}

void prep_encoder::putEmptyTaggedFieldArray()
{
    if (_is_fixed())
    {
    }
    else if (_is_flexible())
    {
        putUVarint(0);
    }
}

int prep_encoder::offset() const
{
    return m_length;
}

void prep_encoder::push(push_encoder &_in)
{
    auto in = &_in;
    in->save_offset(m_length);
    m_length += in->reserve_length();
    m_stack.push_back(in);
}

int prep_encoder::pop()
{
    if (m_stack.empty())
        return -1;

    auto in = m_stack.back();
    m_stack.pop_back();

    auto dpe = dynamic_cast<dynamicPushEncoder *>(in);
    if (dpe != nullptr)
    {
        m_length += dpe->adjust_length(m_length);
    }
    return 0;
}

int prep_encoder::putInt32Array(const std::vector<int32_t> &in)
{
    if (_is_fixed())
    {
    }
    else if (_is_flexible())
    {
        if (in.empty())
            return -1; // non-null required
        putUVarint(static_cast<uint64_t>(in.size()) + 1);
        m_length += 4 * static_cast<int>(in.size());
        return 0;
    }
    return 0;
}
