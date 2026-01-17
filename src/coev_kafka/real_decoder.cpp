#include "real_decoder.h"
#include "response_header.h"
#include <cstdint>
#include <cstring>
#include <limits>
#include <coev/coev.h>

int realDecoder::getInt8(int8_t &result)
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

int realDecoder::getInt16(int16_t &result)
{
    if (remaining() < 2)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int16_t>((m_raw[m_offset] << 8) | m_raw[m_offset + 1]);
    m_offset += 2;
    return 0;
}

int realDecoder::getInt32(int32_t &result)
{
    if (remaining() < 4)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int32_t>((m_raw[m_offset] << 24) | (m_raw[m_offset + 1] << 16) | (m_raw[m_offset + 2] << 8) | m_raw[m_offset + 3]);
    m_offset += 4;
    return 0;
}

int realDecoder::getInt64(int64_t &result)
{
    if (remaining() < 8)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int64_t>(
        (static_cast<uint64_t>(m_raw[m_offset]) << 56) |
        (static_cast<uint64_t>(m_raw[m_offset + 1]) << 48) |
        (static_cast<uint64_t>(m_raw[m_offset + 2]) << 40) |
        (static_cast<uint64_t>(m_raw[m_offset + 3]) << 32) |
        (static_cast<uint64_t>(m_raw[m_offset + 4]) << 24) |
        (static_cast<uint64_t>(m_raw[m_offset + 5]) << 16) |
        (static_cast<uint64_t>(m_raw[m_offset + 6]) << 8) |
        static_cast<uint64_t>(m_raw[m_offset + 7]));
    m_offset += 8;
    return 0;
}

int realDecoder::getVariant(int64_t &result)
{
    uint64_t ux;
    int shift = 0;
    result = 0;
    for (int i = 0; i < 10; i++)
    {
        if (m_offset >= m_raw.size())
        {
            m_offset = m_raw.size();
            result = -1;
            return ErrInsufficientData;
        }
        uint8_t b = m_raw[m_offset];
        m_offset++;
        ux = static_cast<uint64_t>(b & 0x7F) << shift;
        result |= static_cast<int64_t>(ux);
        if ((b & 0x80) == 0)
        {
            if (i == 9 && ((ux & 0xFE00000000000000ULL) != 0 || (b & 0x7F) > 1))
            {
                m_offset -= (i + 1);
                result = -1;
                return ErrVariantOverflow;
            }
            return 0;
        }
        shift += 7;
    }
    m_offset -= 10;
    result = -1;
    return ErrVariantOverflow;
}

int realDecoder::getUVariant(uint64_t &result)
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
        uint8_t b = m_raw[m_offset];
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

int realDecoder::getFloat64(double &result)
{
    if (remaining() < 8)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    uint64_t bits = static_cast<uint64_t>(m_raw[m_offset]) << 56 |
                    static_cast<uint64_t>(m_raw[m_offset + 1]) << 48 |
                    static_cast<uint64_t>(m_raw[m_offset + 2]) << 40 |
                    static_cast<uint64_t>(m_raw[m_offset + 3]) << 32 |
                    static_cast<uint64_t>(m_raw[m_offset + 4]) << 24 |
                    static_cast<uint64_t>(m_raw[m_offset + 5]) << 16 |
                    static_cast<uint64_t>(m_raw[m_offset + 6]) << 8 |
                    static_cast<uint64_t>(m_raw[m_offset + 7]);
    result = *reinterpret_cast<double *>(&bits);
    m_offset += 8;
    return 0;
}

int realDecoder::getArrayLength(int32_t &result)
{
    if (remaining() < 4)
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    result = static_cast<int32_t>((m_raw[m_offset] << 24) | (m_raw[m_offset + 1] << 16) | (m_raw[m_offset + 2] << 8) | m_raw[m_offset + 3]);
    m_offset += 4;
    if (result > remaining())
    {
        m_offset = m_raw.size();
        result = -1;
        return ErrInsufficientData;
    }
    else if (result > MaxResponseSize)
    {
        result = -1;
        return ErrInvalidArrayLength;
    }
    return 0;
}

int realDecoder::getBool(bool &result)
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

int realDecoder::getKError(KError &result)
{
    int16_t out;
    auto err = getInt16(out);
    result = KError(out);
    return err;
}

int realDecoder::getDurationMs(std::chrono::milliseconds &out)
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

int realDecoder::getTaggedFieldArray(const taggedFieldDecoders &decoders)
{

    return ErrTaggedFieldsInNonFlexibleContext;
}

int realDecoder::getEmptyTaggedFieldArray(int32_t &result)
{
    result = 0;
    return 0;
}

int realDecoder::getBytes(std::string &result)
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

int realDecoder::getVariantBytes(std::string &result)
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

int realDecoder::getStringLength(int &result)
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

int realDecoder::getString(std::string &result)
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

int realDecoder::getNullableString(std::string &result)
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

int realDecoder::getInt32Array(std::vector<int32_t> &result)
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

int realDecoder::getInt64Array(std::vector<int64_t> &result)
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
            (static_cast<uint64_t>(m_raw[m_offset]) << 56) |
            (static_cast<uint64_t>(m_raw[m_offset + 1]) << 48) |
            (static_cast<uint64_t>(m_raw[m_offset + 2]) << 40) |
            (static_cast<uint64_t>(m_raw[m_offset + 3]) << 32) |
            (static_cast<uint64_t>(m_raw[m_offset + 4]) << 24) |
            (static_cast<uint64_t>(m_raw[m_offset + 5]) << 16) |
            (static_cast<uint64_t>(m_raw[m_offset + 6]) << 8) |
            static_cast<uint64_t>(m_raw[m_offset + 7]));
        m_offset += 8;
    }
    return 0;
}

int realDecoder::getStringArray(std::vector<std::string> &result)
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

int realDecoder::remaining()
{
    return static_cast<int>(m_raw.size()) - m_offset;
}

int realDecoder::getSubset(int length, std::shared_ptr<packetDecoder> &out)
{
    std::string buf;
    int err = getRawBytes(length, buf);
    if (err != 0)
    {
        out = nullptr;
        return err;
    }
    auto decoder = std::make_shared<realDecoder>();
    decoder->m_raw = std::move(buf);
    out = decoder;
    return 0;
}

int realDecoder::getRawBytes(int length, std::string &result)
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

int realDecoder::peek(int offset, int length, std::shared_ptr<packetDecoder> &result)
{
    if (remaining() < offset + length)
    {
        result = nullptr;
        return ErrInsufficientData;
    }
    int start = m_offset + offset;
    auto decoder = std::make_shared<realDecoder>();
    decoder->m_raw.assign(m_raw.begin() + start, m_raw.begin() + start + length);
    result = decoder;
    return 0;
}

int realDecoder::peekInt8(int offset, int8_t &result)
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

int realDecoder::push(std::shared_ptr<pushDecoder> in)
{
    in->save_offset(m_offset);
    int reserve = 0;
    auto dpd = std::dynamic_pointer_cast<dynamicPushDecoder>(in);
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

int realDecoder::pop()
{
    auto in = m_stack.back();
    m_stack.pop_back();
    return in->check(m_offset, m_raw);
}

int realFlexibleDecoder::getArrayLength(int32_t &result)
{
    uint64_t n;
    int err = getUVariant(n);
    if (err != 0)
    {
        result = 0;
        return err;
    }
    if (n == 0)
    {
        result = 0;
        return 0;
    }
    result = static_cast<int32_t>(n - 1);
    return 0;
}

int realFlexibleDecoder::getEmptyTaggedFieldArray(int32_t &result)
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

int realFlexibleDecoder::getTaggedFieldArray(const taggedFieldDecoders &decoders)
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
        auto flex_decoder = std::make_shared<realFlexibleDecoder>();
        flex_decoder->m_raw = bytes;
        err = decoder(*flex_decoder);
        if (err != 0)
        {
            return err;
        }
    }
    return 0;
}

int realFlexibleDecoder::getBytes(std::string &result)
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

int realFlexibleDecoder::getStringLength(int &result)
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

int realFlexibleDecoder::getString(std::string &result)
{
    int length;
    int err = getStringLength(length);
    if (err != 0 || length == -1)
    {
        result = "";
        return err;
    }
    if (length < 0)
    {
        result = "";
        return ErrInvalidStringLength;
    }
    result.assign(reinterpret_cast<const char *>(m_raw.data() + m_offset), length);
    m_offset += length;
    return 0;
}

int realFlexibleDecoder::getNullableString(std::string &result)
{
    int length;
    int err = getStringLength(length);
    if (err != 0)
    {
        result = "";
        return err;
    }
    if (length < 0)
    {
        result = "";
        return err;
    }
    result.assign(reinterpret_cast<const char *>(m_raw.data() + m_offset), length);
    m_offset += length;
    return 0;
}

int realFlexibleDecoder::getInt32Array(std::vector<int32_t> &result)
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

int realFlexibleDecoder::getStringArray(std::vector<std::string> &result)
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