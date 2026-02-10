#include "real_encoder.h"
#include <cassert>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <algorithm>

static inline uint64_t zigzagEncode(int64_t x)
{
    return (static_cast<uint64_t>(x) << 1) ^ static_cast<uint64_t>(-(x < 0));
}
static int encodeUVariant(uint8_t *buf, uint64_t x)
{
    int n = 0;
    while (x >= 0x80)
    {
        buf[n++] = static_cast<uint8_t>(x) | 0x80;
        x >>= 7;
    }
    buf[n++] = static_cast<uint8_t>(x);
    return n;
}
int encodeVariant(uint8_t *buf, int64_t x)
{
    return encodeUVariant(buf, zigzagEncode(x));
}

void real_encoder::putInt8(int8_t in)
{
    assert(m_offset + 1 <= m_raw.size());
    m_raw[m_offset++] = static_cast<uint8_t>(in);
}

void real_encoder::putInt16(int16_t in)
{
    assert(m_offset + 2 <= m_raw.size());
    m_raw[m_offset] = static_cast<uint8_t>((in >> 8) & 0xFF);
    m_raw[m_offset + 1] = static_cast<uint8_t>(in & 0xFF);
    m_offset += 2;
}

void real_encoder::putInt32(int32_t in)
{
    assert(m_offset + 4 <= m_raw.size());
    m_raw[m_offset] = static_cast<uint8_t>((in >> 24) & 0xFF);
    m_raw[m_offset + 1] = static_cast<uint8_t>((in >> 16) & 0xFF);
    m_raw[m_offset + 2] = static_cast<uint8_t>((in >> 8) & 0xFF);
    m_raw[m_offset + 3] = static_cast<uint8_t>(in & 0xFF);
    m_offset += 4;
}

void real_encoder::putInt64(int64_t in)
{
    assert(m_offset + 8 <= m_raw.size());
    m_raw[m_offset] = static_cast<uint8_t>((in >> 56) & 0xFF);
    m_raw[m_offset + 1] = static_cast<uint8_t>((in >> 48) & 0xFF);
    m_raw[m_offset + 2] = static_cast<uint8_t>((in >> 40) & 0xFF);
    m_raw[m_offset + 3] = static_cast<uint8_t>((in >> 32) & 0xFF);
    m_raw[m_offset + 4] = static_cast<uint8_t>((in >> 24) & 0xFF);
    m_raw[m_offset + 5] = static_cast<uint8_t>((in >> 16) & 0xFF);
    m_raw[m_offset + 6] = static_cast<uint8_t>((in >> 8) & 0xFF);
    m_raw[m_offset + 7] = static_cast<uint8_t>(in & 0xFF);
    m_offset += 8;
}

void real_encoder::putVariant(int64_t in)
{
    m_offset += encodeVariant((uint8_t *)m_raw.data() + m_offset, in);
    assert(m_offset <= m_raw.size());
}

void real_encoder::putUVarint(uint64_t in)
{
    m_offset += encodeUVariant((uint8_t *)m_raw.data() + m_offset, in);
    assert(m_offset <= m_raw.size());
}

void real_encoder::putFloat64(double in)
{
    uint64_t bits = *reinterpret_cast<uint64_t *>(&in);
    putInt64(static_cast<int64_t>(bits));
}

int real_encoder::putArrayLength(int in)
{
    if (isFixed())
    {
        putInt32(static_cast<int32_t>(in));
    }
    else if (isFlexible())
    {
        putUVarint(static_cast<uint64_t>(in + 1));
    }
    return 0;
}

void real_encoder::putBool(bool in)
{
    putInt8(in ? 1 : 0);
}

void real_encoder::putKError(KError in)
{
    putInt16(static_cast<int16_t>(in));
}

void real_encoder::putDurationMs(std::chrono::milliseconds ms)
{
    putInt32(static_cast<int32_t>(ms.count()));
}

int real_encoder::putRawBytes(const std::string_view &in)
{
    assert(m_offset + in.size() <= m_raw.size());
    std::memcpy(&m_raw[m_offset], in.data(), in.size());
    m_offset += in.size();
    return 0;
}

int real_encoder::putBytes(const std::string_view &in)
{
    if (isFixed())
    {
        if (in.empty())
        {
            putInt32(-1);
            return 0;
        }
        putInt32(static_cast<int32_t>(in.size()));
        putRawBytes(in);
        return 0;
    }
    else if (isFlexible())
    {
        putUVarint(static_cast<uint64_t>(in.size() + 1));
        return putRawBytes(in);
    }
    return 0;
}

int real_encoder::putVariantBytes(const std::string_view &in)
{
    if (in.empty())
    {
        putVariant(-1);
        return 0;
    }
    putVariant(static_cast<int64_t>(in.size()));
    putRawBytes(in);
    return 0;
}

int real_encoder::putString(const std::string_view &in)
{
    if (isFixed())
    {
        putInt16(static_cast<int16_t>(in.size()));
        assert(m_offset + in.size() <= m_raw.size());
        std::memcpy(&m_raw[m_offset], in.data(), in.size());
        m_offset += in.size();
        return 0;
    }
    else if (isFlexible())
    {
        putArrayLength(static_cast<uint64_t>(in.size()));
        putRawBytes(in);
        return 0;
    }
    return 0;
}

int real_encoder::putNullableString(const std::string_view &in)
{
    if (isFixed())
    {
        if (in.empty())
        {
            putInt16(-1);
            return 0;
        }
        putString(in);
        return 0;
    }
    else if (isFlexible())
    {
        if (in.empty())
        {
            putUVarint(0);
            return 0;
        }
        return putString(in);
    }
    return 0;
}

int real_encoder::putStringArray(const std::vector<std::string> &in)
{
    if (isFixed())
    {
        putArrayLength(static_cast<int>(in.size()));
        for (auto &s : in)
        {
            putString(s);
        }
        return 0;
    }
    else if (isFlexible())
    {
        putArrayLength(static_cast<int>(in.size()));
        for (auto &s : in)
        {
            putString(s);
        }
        return 0;
    }
    return 0;
}

int real_encoder::putInt32Array(const std::vector<int32_t> &in)
{
    if (isFixed())
    {
        putArrayLength(static_cast<int>(in.size()));
        for (int32_t v : in)
        {
            putInt32(v);
        }
        return 0;
    }
    else if (isFlexible())
    {
        putUVarint(static_cast<uint64_t>(in.size()) + 1);
        for (int32_t v : in)
        {
            putInt32(v);
        }
        return 0;
    }
    return 0;
}

int real_encoder::putNullableInt32Array(const std::vector<int32_t> &in)
{
    if (isFixed())
    {
        if (in.empty())
        {
            putInt32(-1);
            return 0;
        }
        putArrayLength(static_cast<int>(in.size()));
        for (int32_t v : in)
        {
            putInt32(v);
        }
        return 0;
    }
    else if (isFlexible())
    {
        if (in.empty())
        {
            putUVarint(0);
            return 0;
        }
        putUVarint(static_cast<uint64_t>(in.size()) + 1);
        for (int32_t v : in)
        {
            putInt32(v);
        }
        return 0;
    }
    return 0;
}

int real_encoder::putInt64Array(const std::vector<int64_t> &in)
{
    putArrayLength(static_cast<int>(in.size()));
    for (int64_t v : in)
    {
        putInt64(v);
    }
    return 0;
}

void real_encoder::putEmptyTaggedFieldArray()
{
    if (isFixed())
    {
    }
    else if (isFlexible())
    {
        putUVarint(0);
    }
}

void real_encoder::push(push_encoder &in)
{
    in.save_offset(m_offset);
    int reserve = in.reserve_length();
    assert(m_offset + reserve <= m_raw.size());
    m_offset += reserve;
    m_stack.push_back(&in);
}

int real_encoder::pop()
{
    if (m_stack.empty())
    {
        return 0;
    }
    auto in = m_stack.back();
    m_stack.pop_back();
    in->run(m_offset, m_raw);
    return 0;
}
