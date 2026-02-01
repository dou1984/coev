#include "real_decoder.h"
#include "response_header.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <coev/coev.h>

int real_decoder::getInt8(int8_t &result)
{
    if (remaining() < 1)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int8_t>(m_raw[m_offset]);
    m_offset++;
    return 0;
}

int real_decoder::getInt16(int16_t &result)
{
    if (remaining() < 2)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int16_t>((static_cast<uint8_t>(m_raw[m_offset]) << 8) | static_cast<uint8_t>(m_raw[m_offset + 1]));
    m_offset += 2;
    return 0;
}

int real_decoder::getInt32(int32_t &result)
{
    if (remaining() < 4)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int32_t>((static_cast<uint8_t>(m_raw[m_offset]) << 24) | (static_cast<uint8_t>(m_raw[m_offset + 1]) << 16) | (static_cast<uint8_t>(m_raw[m_offset + 2]) << 8) | static_cast<uint8_t>(m_raw[m_offset + 3]));
    m_offset += 4;
    return 0;
}

int real_decoder::getInt64(int64_t &result)
{
    if (remaining() < 8)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int64_t>(
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset])) << 56) |
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 1])) << 48) |
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 2])) << 40) |
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 3])) << 32) |
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 4])) << 24) |
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 5])) << 16) |
        (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 6])) << 8) |
        static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 7])));
    m_offset += 8;
    return 0;
}

static inline int64_t zigzagDecode(uint64_t x)
{
    return static_cast<int64_t>((x >> 1) ^ -(x & 1));
}

int real_decoder::getVariant(int64_t &result)
{
    uint64_t ux = 0;
    int shift = 0;
    for (int i = 0; i < 10; i++)
    {
        if (m_offset >= m_raw.size())
        {
            m_offset = m_raw.size();
            result = -1;
            return ErrInsufficientData;
        }
        uint8_t b = static_cast<uint8_t>(m_raw[m_offset]);
        m_offset++;
        ux |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0)
        {
            if (i == 9 && ((ux & 0xFE00000000000000ULL) != 0 || (b & 0x7F) > 1))
            {
                m_offset -= (i + 1);
                result = -1;
                return ErrVariantOverflow;
            }
            result = zigzagDecode(ux);
            return 0;
        }
        shift += 7;
    }
    m_offset -= 10;
    result = -1;
    return ErrVariantOverflow;
}

int real_decoder::getUVariant(uint64_t &result)
{
    result = 0;
    int shift = 0;
    for (int i = 0; i < 10; i++)
    {
        if (m_offset >= m_raw.size())
        {
            m_offset = m_raw.size();
            result = 0;
            return ErrInsufficientData;
        }
        uint8_t b = static_cast<uint8_t>(m_raw[m_offset]);
        m_offset++;
        result |= static_cast<uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0)
        {
            if (i == 9 && result >> 63)
            {
                m_offset -= (i + 1);
                result = 0;
                return ErrUVariantOverflow;
            }
            return 0;
        }
        shift += 7;
    }
    m_offset -= 10;
    result = 0;
    return ErrUVariantOverflow;
}

int real_decoder::getFloat64(double &result)
{
    if (remaining() < 8)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    uint64_t bits = static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset])) << 56 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 1])) << 48 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 2])) << 40 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 3])) << 32 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 4])) << 24 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 5])) << 16 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 6])) << 8 |
                    static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 7]));
    result = *reinterpret_cast<double *>(&bits);
    m_offset += 8;
    return 0;
}

int real_decoder::getArrayLength(int32_t &result)
{
    if (isFixed())
    {
        if (remaining() < 4)
        {
            m_offset = m_raw.size();
            result = -1;
            return ErrInsufficientData;
        }
        result = static_cast<int32_t>((m_raw[m_offset] << 24) | (m_raw[m_offset + 1] << 16) | (m_raw[m_offset + 2] << 8) | m_raw[m_offset + 3]);
        m_offset += 4;
        if (result > MaxResponseSize)
        {
            result = -1;
            return ErrInvalidArrayLength;
        }
        return 0;
    }
    else if (isFlexible())
    {
        uint64_t n;
        int err = getUVariant(n);
        if (err != 0)
        {
            result = 0;
            return err;
        }
        // According to Kafka flexible protocol, arrays are encoded with size+1
        if (n == 0)
        {
            result = 0;
        }
        else
        {
            result = static_cast<int32_t>(n - 1);
        }
        return 0;
    }
    return 0;
}

int real_decoder::getBool(bool &result)
{
    int8_t b;
    int err = getInt8(b);
    if (err != 0 || b == 0)
    {
        result = false;
        return err;
    }
    if (b != 1)
    {
        result = false;
        return ErrInvalidBool;
    }
    result = true;
    return 0;
}

int real_decoder::getKError(KError &result)
{
    int16_t out;
    auto err = getInt16(out);
    result = KError(out);
    return err;
}

int real_decoder::getDurationMs(std::chrono::milliseconds &out)
{
    int32_t t;
    int err = getInt32(t);
    if (err != 0)
    {
        out = std::chrono::milliseconds(0);
        return err;
    }
    out = std::chrono::milliseconds(t);
    return 0;
}

int real_decoder::getTaggedFieldArray(const taggedFieldDecoders &decoders)
{
    if (isFixed())
    {
        return ErrTaggedFieldsInNonFlexibleContext;
    }
    else if (isFlexible())
    {
        if (decoders.empty())
        {
            int result;
            return getEmptyTaggedFieldArray(result);
        }
        uint64_t tag_count;
        int err = getUVariant(tag_count);
        if (err != 0)
        {
            return err;
        }
        for (uint64_t i = 0; i < tag_count; i++)
        {
            uint64_t id;
            err = getUVariant(id);
            if (err != 0)
            {
                return err;
            }
            uint64_t length;
            err = getUVariant(length);
            if (err != 0)
            {
                return err;
            }
            std::string bytes;
            err = getRawBytes(static_cast<int>(length), bytes);
            if (err != 0)
            {
                return err;
            }
            auto it = decoders.find(id);
            if (it == decoders.end())
            {
                continue;
            }
            auto decoder = it->second;
            pushFlexible();
            defer(popFlexible());
            err = decoder(*this);
            if (err != 0)
            {
                return err;
            }
        }
        return 0;
    }
    return 0;
}

int real_decoder::getEmptyTaggedFieldArray(int32_t &result)
{
    if (isFixed())
    {
        result = 0;
        return 0;
    }
    else if (isFlexible())
    {
        uint64_t tag_count;
        int err = getUVariant(tag_count);
        if (err != 0)
        {
            result = 0;
            return err;
        }
        for (uint64_t i = 0; i < tag_count; i++)
        {
            uint64_t tag_id;
            err = getUVariant(tag_id);
            if (err != 0)
            {
                result = 0;
                return err;
            }
            uint64_t length;
            err = getUVariant(length);
            if (err != 0)
            {
                result = 0;
                return err;
            }
            std::string bytes;
            err = getRawBytes(static_cast<int>(length), bytes);
            if (err != 0)
            {
                result = 0;
                return err;
            }
        }
        result = 0;
        return 0;
    }
    return 0;
}

int real_decoder::getBytes(std::string &result)
{
    if (isFixed())
    {
        int32_t tmp;
        int err = getInt32(tmp);
        if (err != 0)
        {
            return err;
        }
        if (tmp == -1)
        {
            result.clear();
            return 0;
        }
        return getRawBytes(static_cast<int>(tmp), result);
    }
    else if (isFlexible())
    {
        uint64_t n;
        int err = getUVariant(n);
        if (err != 0)
        {
            return err;
        }
        int length = static_cast<int>(n - 1);
        return getRawBytes(length, result);
    }
    return 0;
}

int real_decoder::getVariantBytes(std::string &result)
{
    int64_t tmp;
    int err = getVariant(tmp);
    if (err != 0)
    {
        return err;
    }
    if (tmp == -1)
    {
        result.clear();
        return 0;
    }
    return getRawBytes(static_cast<int>(tmp), result);
}

int real_decoder::getStringLength(int &result)
{
    if (isFixed())
    {
        int16_t length;
        int err = getInt16(length);
        if (err != 0)
        {
            result = 0;
            return err;
        }
        int n = static_cast<int>(length);
        if (n < -1)
        {
            result = 0;
            return ErrInvalidStringLength;
        }
        else if (n > remaining())
        {
            m_offset = m_raw.size();
            result = 0;
            return ErrInsufficientData;
        }
        result = n;
        return 0;
    }
    else if (isFlexible())
    {
        uint64_t length;
        int err = getUVariant(length);
        if (err != 0)
        {
            result = 0;
            return err;
        }
        int n = static_cast<int>(length - 1);
        if (n < -1)
        {
            result = 0;
            return ErrInvalidStringLength;
        }
        else if (n > remaining())
        {
            m_offset = m_raw.size();
            result = 0;
            return ErrInsufficientData;
        }
        result = n;
        return 0;
    }
    return 0;
}

int real_decoder::getString(std::string &result)
{
    if (isFixed())
    {
        int length;
        int err = getStringLength(length);
        if (err != 0 || length == -1)
        {
            result = "";
            return err;
        }
        result.assign(reinterpret_cast<const char *>(m_raw.data() + m_offset), length);
        m_offset += length;
        return 0;
    }
    else if (isFlexible())
    {
        int length;
        int err = getStringLength(length);
        if (err != 0 || length == -1)
        {
            result = "";
            return err;
        }
        result.assign(reinterpret_cast<const char *>(m_raw.data() + m_offset), length);
        m_offset += length;
        return 0;
    }
    return 0;
}

int real_decoder::getNullableString(std::string &result)
{
    if (isFixed())
    {
        int n;
        int err = getStringLength(n);
        if (err != 0 || n == -1)
        {
            result = "";
            return err;
        }
        result.assign(reinterpret_cast<const char *>(m_raw.data() + m_offset), n);
        m_offset += n;
        return 0;
    }
    else if (isFlexible())
    {
        int length;
        int err = getStringLength(length);
        if (err != 0 || length == -1)
        {
            result = "";
            return err;
        }
        result.assign(reinterpret_cast<const char *>(m_raw.data() + m_offset), length);
        m_offset += length;
        return 0;
    }
    return 0;
}

int real_decoder::getInt32Array(std::vector<int32_t> &result)
{
    if (isFixed())
    {
        int32_t n;
        int err = getArrayLength(n);
        if (err != 0)
        {
            return err;
        }
        if (n <= 0)
        {
            result.clear();
            return 0;
        }
        if (remaining() < 4 * n)
        {
            m_offset = m_raw.size();
            return ErrInsufficientData;
        }
        result.resize(n);
        for (int32_t i = 0; i < n; i++)
        {
            result[i] = static_cast<int32_t>((m_raw[m_offset] << 24) | (m_raw[m_offset + 1] << 16) | (m_raw[m_offset + 2] << 8) | m_raw[m_offset + 3]);
            m_offset += 4;
        }
        return 0;
    }
    else if (isFlexible())
    {
        uint64_t n;
        int err = getUVariant(n);
        if (err != 0)
        {
            return err;
        }
        if (n == 0)
        {
            result.clear();
            return 0;
        }
        int array_length = static_cast<int>(n - 1);
        result.resize(array_length);
        for (int i = 0; i < array_length; i++)
        {
            result[i] = static_cast<int32_t>((m_raw[m_offset] << 24) | (m_raw[m_offset + 1] << 16) | (m_raw[m_offset + 2] << 8) | m_raw[m_offset + 3]);
            m_offset += 4;
        }
        return 0;
    }
    return 0;
}

int real_decoder::getInt64Array(std::vector<int64_t> &result)
{
    int32_t n;
    int err = getArrayLength(n);
    if (err != 0)
    {
        return err;
    }
    if (n <= 0)
    {
        result.clear();
        return 0;
    }
    if (remaining() < 8 * n)
    {
        m_offset = m_raw.size();
        return ErrInsufficientData;
    }
    result.resize(n);
    for (int32_t i = 0; i < n; i++)
    {
        result[i] = static_cast<int64_t>(
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset])) << 56) |
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 1])) << 48) |
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 2])) << 40) |
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 3])) << 32) |
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 4])) << 24) |
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 5])) << 16) |
            (static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 6])) << 8) |
            static_cast<uint64_t>(static_cast<uint8_t>(m_raw[m_offset + 7])));
        m_offset += 8;
    }
    return 0;
}

int real_decoder::getStringArray(std::vector<std::string> &result)
{
    if (isFixed())
    {
        int32_t n;
        int err = getArrayLength(n);
        if (err != 0)
        {
            return err;
        }
        if (n <= 0)
        {
            result.clear();
            return 0;
        }
        result.resize(n);
        for (int32_t i = 0; i < n; i++)
        {
            std::string str;
            err = getString(str);
            if (err != 0)
            {
                return err;
            }
            result[i] = str;
        }
        return 0;
    }
    else if (isFlexible())
    {
        int32_t n;
        int err = getArrayLength(n);
        if (err != 0)
        {
            return err;
        }
        if (n <= 0)
        {
            result.clear();
            return 0;
        }
        result.resize(n);
        for (int32_t i = 0; i < n; i++)
        {
            std::string str;
            err = getString(str);
            if (err != 0)
            {
                return err;
            }
            result[i] = str;
        }
        return 0;
    }
    return 0;
}

int real_decoder::remaining()
{
    return static_cast<int>(m_raw.size()) - m_offset;
}

int real_decoder::getSubset(int length, std::shared_ptr<packet_decoder> &out)
{
    std::string buf;
    int err = getRawBytes(length, buf);
    if (err != 0)
    {
        out = nullptr;
        return err;
    }
    auto decoder = std::make_shared<real_decoder>();
    decoder->m_raw = std::move(buf);
    out = decoder;
    return 0;
}

int real_decoder::getRawBytes(int length, std::string &result)
{
    if (length < 0)
    {
        return ErrValidByteSliceLength;
    }
    else if (length > remaining())
    {
        m_offset = m_raw.size();
        return ErrInsufficientData;
    }
    result.assign(m_raw.begin() + m_offset, m_raw.begin() + m_offset + length);
    m_offset += length;
    return 0;
}

int real_decoder::peek(int offset, int length, std::shared_ptr<packet_decoder> &result)
{
    if (remaining() < offset + length)
    {
        result = nullptr;
        return ErrInsufficientData;
    }
    int start = m_offset + offset;
    auto decoder = std::make_shared<real_decoder>();
    decoder->m_raw = std::string_view(m_raw.begin() + start, m_raw.begin() + start + length);
    result = decoder;
    return 0;
}

int real_decoder::peekInt8(int offset, int8_t &result)
{
    const int byte_len = 1;
    if (m_offset + offset >= m_raw.size())
    {
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int8_t>(m_raw[m_offset + offset]);
    return 0;
}

int real_decoder::push(push_decoder &_in)
{
    auto in = &_in;
    in->save_offset(m_offset);
    int reserve = 0;
    auto dpd = dynamic_cast<dynamicPushDecoder *>(in);
    if (dpd != nullptr)
    {
        int err = dpd->decode(*this);
        if (err != 0)
        {
            return err;
        }
    }
    else
    {
        reserve = in->reserve_length();
        if (remaining() < reserve)
        {
            m_offset = m_raw.size();
            return ErrInsufficientData;
        }
    }
    m_stack.push_back(in);
    m_offset += reserve;
    return 0;
}

int real_decoder::pop()
{
    auto in = m_stack.back();
    m_stack.pop_back();
    return in->check(m_offset, m_raw);
}
