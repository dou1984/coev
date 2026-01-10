#include "real_encoder.h"
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

void realEncoder::putInt8(int8_t in)
{
    m_raw[m_offset++] = static_cast<uint8_t>(in);
}

void realEncoder::putInt16(int16_t in)
{
    m_raw[m_offset] = static_cast<uint8_t>((in >> 8) & 0xFF);
    m_raw[m_offset + 1] = static_cast<uint8_t>(in & 0xFF);
    m_offset += 2;
}

void realEncoder::putInt32(int32_t in)
{
    m_raw[m_offset] = static_cast<uint8_t>((in >> 24) & 0xFF);
    m_raw[m_offset + 1] = static_cast<uint8_t>((in >> 16) & 0xFF);
    m_raw[m_offset + 2] = static_cast<uint8_t>((in >> 8) & 0xFF);
    m_raw[m_offset + 3] = static_cast<uint8_t>(in & 0xFF);
    m_offset += 4;
}

void realEncoder::putInt64(int64_t in)
{
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

void realEncoder::putVariant(int64_t in)
{
    m_offset += encodeVariant((uint8_t *)m_raw.data() + m_offset, in);
}

void realEncoder::putUVarint(uint64_t in)
{
    m_offset += encodeUVariant((uint8_t *)m_raw.data() + m_offset, in);
}

void realEncoder::putFloat64(double in)
{
    uint64_t bits = *reinterpret_cast<uint64_t *>(&in);
    putInt64(static_cast<int64_t>(bits));
}

int realEncoder::putArrayLength(int in)
{
    putInt32(static_cast<int32_t>(in));
    return 0;
}

void realEncoder::putBool(bool in)
{
    putInt8(in ? 1 : 0);
}

void realEncoder::putKError(KError in)
{
    putInt16(static_cast<int16_t>(in));
}

void realEncoder::putDurationMs(std::chrono::milliseconds ms)
{
    putInt32(static_cast<int32_t>(ms.count()));
}

int realEncoder::putRawBytes(const std::string &in)
{
    m_raw += in;
    m_offset += in.size();
    return 0;
}

int realEncoder::putBytes(const std::string &in)
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

int realEncoder::putVariantBytes(const std::string &in)
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

int realEncoder::putString(const std::string &in)
{
    putInt16(static_cast<int16_t>(in.size()));
    m_raw += in;
    m_offset += in.size();
    return 0;
}

int realEncoder::putNullableString(const std::string &in)
{
    if (in.empty())
    {
        putInt16(-1);
        return 0;
    }
    putString(in);
    return 0;
}

int realEncoder::putStringArray(const std::vector<std::string> &in)
{
    putArrayLength(static_cast<int>(in.size()));
    for (auto &s : in)
    {
        putString(s);
    }
    return 0;
}

int realEncoder::putInt32Array(const std::vector<int32_t> &in)
{
    putArrayLength(static_cast<int>(in.size()));
    for (int32_t v : in)
    {
        putInt32(v);
    }
    return 0;
}

int realEncoder::putNullableInt32Array(const std::vector<int32_t> &in)
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

int realEncoder::putInt64Array(const std::vector<int64_t> &in)
{
    putArrayLength(static_cast<int>(in.size()));
    for (int64_t v : in)
    {
        putInt64(v);
    }
    return 0;
}

void realEncoder::putEmptyTaggedFieldArray()
{
    // No-op in non-flexible mode
}

void realEncoder::push(std::shared_ptr<pushEncoder> in)
{
    in->saveOffset(m_offset);
    m_offset += in->reserveLength();
    m_stack.push_back(in);
}

int realEncoder::pop()
{
    if (m_stack.empty())
        return 0;
    auto in = m_stack.back();
    m_stack.pop_back();
    in->run(m_offset, m_raw);
    return 0;
}

int realFlexibleEncoder::putArrayLength(int in)
{
    m_base->putUVarint(static_cast<uint64_t>(in + 1));
    return 0;
}

int realFlexibleEncoder::putBytes(const std::string &in)
{
    m_base->putUVarint(static_cast<uint64_t>(in.size() + 1));
    return m_base->putRawBytes(in);
}

int realFlexibleEncoder::putString(const std::string &in)
{
    putArrayLength(static_cast<int>(in.size()));
    m_base->putRawBytes(std::string(in.begin(), in.end()));
    return 0;
}

int realFlexibleEncoder::putNullableString(const std::string &in)
{
    if (in.empty())
    {
        m_base->putInt8(0);
        return 0;
    }
    return putString(in);
}

int realFlexibleEncoder::putStringArray(const std::vector<std::string> &in)
{
    putArrayLength(static_cast<int>(in.size()));
    for (auto &s : in)
    {
        putString(s);
    }
    return 0;
}

int realFlexibleEncoder::putInt32Array(const std::vector<int32_t> &in)
{
    m_base->putUVarint(static_cast<uint64_t>(in.size()) + 1);
    for (int32_t v : in)
    {
        m_base->putInt32(v);
    }
    return 0;
}

int realFlexibleEncoder::putNullableInt32Array(const std::vector<int32_t> &in)
{
    if (in.empty())
    {
        m_base->putUVarint(0);
        return 0;
    }
    m_base->putUVarint(static_cast<uint64_t>(in.size()) + 1);
    for (int32_t v : in)
    {
        m_base->putInt32(v);
    }
    return 0;
}

void realFlexibleEncoder::putEmptyTaggedFieldArray()
{
    m_base->putUVarint(0);
}
