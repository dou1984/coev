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

prepEncoder::prepEncoder() = default;

void prepEncoder::putInt8(int8_t /*in*/)
{
    m_length += 1;
}
void prepEncoder::putInt16(int16_t /*in*/)
{
    m_length += 2;
}
void prepEncoder::putInt32(int32_t /*in*/)
{
    m_length += 4;
}
void prepEncoder::putInt64(int64_t /*in*/)
{
    m_length += 8;
}
void prepEncoder::putVariant(int64_t in)
{
    m_length += varintLen(in);
}
void prepEncoder::putUVarint(uint64_t in)
{
    m_length += uvarintLen(in);
}
void prepEncoder::putFloat64(double /*in*/)
{
    m_length += 8;
}

int prepEncoder::putArrayLength(int32_t in)
{
    if (in > std::numeric_limits<int32_t>::max())
    {
        return -1; // PacketEncodingError equivalent
    }
    m_length += 4;
    return 0;
}

void prepEncoder::putBool(bool /*in*/)
{
    m_length++;
}
void prepEncoder::putKError(KError /*in*/)
{
    m_length += 2;
}
void prepEncoder::putDurationMs(std::chrono::milliseconds /*in*/)
{
    m_length += 4;
}

int prepEncoder::putBytes(const std::string &in)
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

int prepEncoder::putVariantBytes(const std::string &in)
{
    if (in.empty())
    {
        putVariant(-1);
        return 0;
    }
    putVariant(static_cast<int64_t>(in.size()));
    return putRawBytes(in);
}

int prepEncoder::putRawBytes(const std::string &in)
{
    if (in.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
    {
        return -1;
    }
    m_length += static_cast<int>(in.size());
    return 0;
}

int prepEncoder::putString(const std::string &in)
{
    if (in.length() > static_cast<size_t>(std::numeric_limits<int16_t>::max()))
    {
        return -1;
    }
    m_length += 2;
    m_length += static_cast<int>(in.length());
    return 0;
}

int prepEncoder::putNullableString(const std::string &in)
{
    if (in.empty())
    {
        m_length += 2; // null string encoded as -1 (int16)
        return 0;
    }
    return putString(in);
}

int prepEncoder::putStringArray(const std::vector<std::string> &in)
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

int prepEncoder::putInt32Array(const std::vector<int32_t> &in)
{
    int err = putArrayLength(static_cast<int32_t>(in.size()));
    if (err != 0)
        return err;
    m_length += 4 * static_cast<int>(in.size());
    return 0;
}

int prepEncoder::putNullableInt32Array(const std::vector<int32_t> &in)
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

int prepEncoder::putInt64Array(const std::vector<int64_t> &in)
{
    int err = putArrayLength(static_cast<int32_t>(in.size()));
    if (err != 0)
        return err;
    m_length += 8 * static_cast<int>(in.size());
    return 0;
}

void prepEncoder::putEmptyTaggedFieldArray()
{
    // no-op in legacy format
}

int prepEncoder::offset() const
{
    return m_length;
}

void prepEncoder::push(std::shared_ptr<pushEncoder> in)
{
    in->save_offset(m_length);
    m_length += in->reserve_length();
    m_stack.push_back(in);
}

int prepEncoder::pop()
{
    if (m_stack.empty())
        return -1;

    auto in = m_stack.back();
    m_stack.pop_back();

    if (auto dpe = std::dynamic_pointer_cast<dynamicPushEncoder>(in))
    {
        m_length += dpe->adjust_length(m_length);
    }
    return 0;
}

std::shared_ptr<metrics::Registry> prepEncoder::metricRegistry()
{
    return nullptr;
}

prepFlexibleEncoder::prepFlexibleEncoder(const prepEncoder &in)
{
    m_length = in.offset();
    m_stack = in.m_stack;
}
int prepFlexibleEncoder::putArrayLength(int32_t in)
{
    putUVarint(static_cast<uint64_t>(in + 1));
    return 0;
}

int prepFlexibleEncoder::putBytes(const std::string &in)
{
    putUVarint(in.size() + 1);
    return putRawBytes(in);
}

int prepFlexibleEncoder::putString(const std::string &in)
{
    int err = putArrayLength(static_cast<int32_t>(in.length()));
    if (err != 0)
        return err;
    return putRawBytes(in);
}

int prepFlexibleEncoder::putNullableString(const std::string &in)
{
    if (in == "")
    {
        putUVarint(0);
        return 0;
    }
    return putString(in);
}

int prepFlexibleEncoder::putStringArray(const std::vector<std::string> &in)
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

int prepFlexibleEncoder::putInt32Array(const std::vector<int32_t> &in)
{
    if (in.empty())
        return -1; // non-null required
    putUVarint(static_cast<uint64_t>(in.size()) + 1);
    m_length += 4 * static_cast<int>(in.size());
    return 0;
}

int prepFlexibleEncoder::putNullableInt32Array(const std::vector<int32_t> &in)
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

void prepFlexibleEncoder::putEmptyTaggedFieldArray()
{
    putUVarint(0);
}