#include <cstring>
#include <coev/coev.h>
#include "packet_encoder.h"

namespace coev::kafka
{
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
    static inline uint64_t zigzagEncode(int64_t x)
    {
        return (static_cast<uint64_t>(x) << 1) ^ static_cast<uint64_t>(-(x < 0));
    }
    int encodeUVariant(uint8_t *buf, uint64_t x)
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
    packet_encoder::packet_encoder(Type type) : m_type(type)
    {
    }
    packet_encoder::packet_encoder(Type type, size_t l) : m_type(type)
    {
        m_raw.resize(l);
    }
    packet_encoder::packet_encoder(std::string_view buf)
    {
        m_type = REAL;
        m_raw = buf;
    }
    packet_encoder::~packet_encoder()
    {
    }
    void packet_encoder::putInt8(int8_t in)
    {
        if (m_type == PREP)
        {
            m_length += 1;
        }
        else if (m_type == REAL)
        {
            assert(m_offset + 1 <= m_raw.size());
            m_raw[m_offset++] = static_cast<uint8_t>(in);
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putInt16(int16_t in)
    {
        if (m_type == PREP)
        {
            m_length += 2;
        }
        else if (m_type == REAL)
        {
            assert(m_offset + 2 <= m_raw.size());
            m_raw[m_offset] = static_cast<uint8_t>((in >> 8) & 0xFF);
            m_raw[m_offset + 1] = static_cast<uint8_t>(in & 0xFF);
            m_offset += 2;
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putInt32(int32_t in)
    {
        if (m_type == PREP)
        {
            m_length += 4;
        }
        else if (m_type == REAL)
        {
            assert(m_offset + 4 <= m_raw.size());
            m_raw[m_offset] = static_cast<uint8_t>((in >> 24) & 0xFF);
            m_raw[m_offset + 1] = static_cast<uint8_t>((in >> 16) & 0xFF);
            m_raw[m_offset + 2] = static_cast<uint8_t>((in >> 8) & 0xFF);
            m_raw[m_offset + 3] = static_cast<uint8_t>(in & 0xFF);
            m_offset += 4;
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putInt64(int64_t in)
    {
        if (m_type == PREP)
        {
            m_length += 8;
        }
        else if (m_type == REAL)
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
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putVariant(int64_t in)
    {
        if (m_type == PREP)
        {
            m_length += varintLen(in);
        }
        else if (m_type == REAL)
        {
            m_offset += encodeVariant((uint8_t *)m_raw.data() + m_offset, in);
            assert(m_offset <= m_raw.size());
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putUVarint(uint64_t in)
    {
        if (m_type == PREP)
        {
            m_length += uvarintLen(in);
        }
        else if (m_type == REAL)
        {
            m_offset += encodeUVariant((uint8_t *)m_raw.data() + m_offset, in);
            assert(m_offset <= m_raw.size());
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putFloat64(double in)
    {
        if (m_type == PREP)
        {
            m_length += 8;
        }
        else if (m_type == REAL)
        {
            uint64_t bits = *reinterpret_cast<uint64_t *>(&in);
            putInt64(static_cast<int64_t>(bits));
        }
        else
        {
            assert(false);
        }
    }

    int packet_encoder::putArrayLength(int32_t in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
            {
                if (in > std::numeric_limits<int32_t>::max())
                {
                    return -1;
                }
                m_length += 4;
                return 0;
            }
            else if (__is_flexible())
            {
                putUVarint(static_cast<uint64_t>(in + 1));
                return 0;
            }
            return 0;
        }
        else if (m_type == REAL)
        {
            if (__is_fixed())
            {
                putInt32(static_cast<int32_t>(in));
            }
            else if (__is_flexible())
            {
                putUVarint(static_cast<uint64_t>(in + 1));
            }
            return 0;
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    void packet_encoder::putBool(bool in)
    {
        if (m_type == PREP)
        {
            m_length++;
        }
        else if (m_type == REAL)
        {
            putInt8(in ? 1 : 0);
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putKError(KError in)
    {
        if (m_type == PREP)
        {
            m_length += 2;
        }
        else if (m_type == REAL)
        {
            putInt16(static_cast<int16_t>(in));
        }
        else
        {
            assert(false);
        }
    }
    void packet_encoder::putDurationMs(std::chrono::milliseconds ms)
    {
        if (m_type == PREP)
        {
            m_length += 4;
        }
        else if (m_type == REAL)
        {
            putInt32(static_cast<int32_t>(ms.count()));
        }
        else
        {
            assert(false);
        }
    }
    int packet_encoder::putRawBytes(const std::string_view &in)
    {
        if (m_type == PREP)
        {
            if (in.size() > static_cast<size_t>(std::numeric_limits<int32_t>::max()))
            {
                return -1;
            }
            m_length += static_cast<int>(in.size());
            return 0;
        }
        else if (m_type == REAL)
        {
            assert(m_offset + in.size() <= m_raw.size());
            std::memcpy(&m_raw[m_offset], in.data(), in.size());
            m_offset += in.size();
            return 0;
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putBytes(const std::string_view &in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
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
            else if (__is_flexible())
            {
                putUVarint(in.size() + 1);
                return putRawBytes(in);
            }
            return 0;
        }
        else if (m_type == REAL)
        {
            if (__is_fixed())
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
            else if (__is_flexible())
            {
                putUVarint(static_cast<uint64_t>(in.size() + 1));
                return putRawBytes(in);
            }
            return 0;
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putVariantBytes(const std::string_view &in)
    {
        if (m_type == PREP)
        {
            if (in.empty())
            {
                putVariant(-1);
                return 0;
            }
            putVariant(static_cast<int64_t>(in.size()));
            return putRawBytes(in);
        }
        else if (m_type == REAL)
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
        else
        {
            assert(false);
            return -1;
        }
    }

    int packet_encoder::putString(const std::string_view &in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
            {
                if (in.length() > static_cast<size_t>(std::numeric_limits<int16_t>::max()))
                {
                    return -1;
                }
                m_length += 2;
                m_length += static_cast<int>(in.length());
                return 0;
            }
            else if (__is_flexible())
            {
                int err = putArrayLength(static_cast<int32_t>(in.length()));
                if (err != 0)
                    return err;
                return putRawBytes(in);
            }
            return 0;
        }
        else if (m_type == REAL)
        {
            if (__is_fixed())
            {
                putInt16(static_cast<int16_t>(in.size()));
                assert(m_offset + in.size() <= m_raw.size());
                std::memcpy(&m_raw[m_offset], in.data(), in.size());
                m_offset += in.size();
                return 0;
            }
            else if (__is_flexible())
            {
                putArrayLength(static_cast<uint64_t>(in.size()));
                putRawBytes(in);
                return 0;
            }
            return 0;
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putNullableString(const std::string_view &in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
            {
                if (in.empty())
                {
                    m_length += 2; // null string encoded as -1 (int16)
                    return 0;
                }
                return putString(in);
            }
            else if (__is_flexible())
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
        else if (m_type == REAL)
        {
            if (__is_fixed())
            {
                if (in.empty())
                {
                    putInt16(-1);
                    return 0;
                }
                putString(in);
                return 0;
            }
            else if (__is_flexible())
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
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putStringArray(const std::vector<std::string> &in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
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
            else if (__is_flexible())
            {
                int err = putArrayLength(static_cast<int32_t>(in.size()));
                if (err != 0)
                    return err;
                m_length += 4 * static_cast<int>(in.size());
                return 0;
            }
            return 0;
        }
        else if (m_type == REAL)
        {
            if (__is_fixed())
            {
                putArrayLength(static_cast<int>(in.size()));
                for (auto &s : in)
                {
                    putString(s);
                }
                return 0;
            }
            else if (__is_flexible())
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
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putInt32Array(const std::vector<int32_t> &in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
            {
                m_length += 4;                               // int32 array length field (4 bytes)
                m_length += 4 * static_cast<int>(in.size()); // each int32 element (4 bytes each)
            }
            else if (__is_flexible())
            {
                putUVarint(static_cast<uint64_t>(in.size()) + 1);
                m_length += 4 * static_cast<int>(in.size());
                return 0;
            }
            return 0;
        }
        else if (m_type == REAL)
        {
            if (__is_fixed())
            {
                putArrayLength(static_cast<int>(in.size()));
                for (int32_t v : in)
                {
                    putInt32(v);
                }
                return 0;
            }
            else if (__is_flexible())
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
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putInt64Array(const std::vector<int64_t> &in)
    {
        if (m_type == PREP)
        {
            int err = putArrayLength(static_cast<int32_t>(in.size()));
            if (err != 0)
                return err;
            m_length += 8 * static_cast<int>(in.size());
            return 0;
        }
        else if (m_type == REAL)
        {
            putArrayLength(static_cast<int>(in.size()));
            for (int64_t v : in)
            {
                putInt64(v);
            }
            return 0;
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    int packet_encoder::putNullableInt32Array(const std::vector<int32_t> &in)
    {
        if (m_type == PREP)
        {
            if (__is_fixed())
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
            else if (__is_flexible())
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
        else if (m_type == REAL)
        {
            if (__is_fixed())
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
            else if (__is_flexible())
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
        else
        {
            assert(false);
            return -1;
        }
    }
    void packet_encoder::putEmptyTaggedFieldArray()
    {
        if (m_type == PREP)
        {

            if (__is_flexible())
            {
                putUVarint(0);
            }
        }
        else if (m_type == REAL)
        {
            if (__is_flexible())
            {
                putUVarint(0);
            }
        }
        else
        {
            assert(false);
        }
    }
    int packet_encoder::offset() const
    {
        if (m_type == PREP)
        {
            return m_length;
        }
        else if (m_type == REAL)
        {
            return static_cast<int>(m_offset);
        }
        else
        {
            assert(false);
            return -1;
        }
    }
    void packet_encoder::push(push_encoder &in)
    {
        if (m_type == PREP)
        {
            in.save_offset(m_length);
            m_length += in.reserve_length();
            m_stack.push_back(&in);
        }
        else if (m_type == REAL)
        {
            in.save_offset(m_offset);
            int reserve = in.reserve_length();
            assert(m_offset + reserve <= m_raw.size());
            m_offset += reserve;
            m_stack.push_back(&in);
        }
        else
        {
            assert(false);
        }
    }
    int packet_encoder::pop()
    {
        if (m_type == PREP)
        {
            if (m_stack.empty())
            {
                return -1;
            }

            auto in = m_stack.back();
            m_stack.pop_back();

            auto dpe = dynamic_cast<dynamicPushEncoder *>(in);
            if (dpe != nullptr)
            {
                m_length += dpe->adjust_length(m_length);
            }
            return 0;
        }
        else if (m_type == REAL)
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
        else
        {
            assert(false);
            return -1;
        }
    }

}