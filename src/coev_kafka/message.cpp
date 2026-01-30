#include <cstring>
#include <map>
#include <utils/hash/crc32.h>
#include "message.h"
#include "message_set.h"
#include "timestamp.h"
#include "real_decoder.h"
#include "undefined.h"
#include "crc32_field.h"

std::string toString(CompressionCodec codec)
{
    switch (codec)
    {
    case CompressionCodec::None:
        return "none";
    case CompressionCodec::GZIP:
        return "gzip";
    case CompressionCodec::Snappy:
        return "snappy";
    case CompressionCodec::LZ4:
        return "lz4";
    case CompressionCodec::ZSTD:
        return "zstd";
    default:
        return "unknown";
    }
}

bool fromString(const std::string &s, CompressionCodec &out)
{
    static const std::map<std::string, CompressionCodec> map = {
        {"none", CompressionCodec::None},
        {"gzip", CompressionCodec::GZIP},
        {"snappy", CompressionCodec::Snappy},
        {"lz4", CompressionCodec::LZ4},
        {"zstd", CompressionCodec::ZSTD}};
    auto it = map.find(s);
    if (it != map.end())
    {
        out = it->second;
        return true;
    }
    return false;
}

std::string compress(CompressionCodec codec, int level, const std::string &data)
{
    std::string out;
    switch (codec)
    {
    case GZIP:
        coev::gzip::Compress(out, data.data(), data.size());
        return out;
    case Snappy:
        coev::snappy::Compress(out, data.data(), data.size());
        return out;
    case LZ4:
        coev::lz4::Compress(out, data.data(), data.size());
        return out;
    case ZSTD:
        coev::zstd::Compress(out, data.data(), data.size());
        return out;
    case None:
        return data;
    }
    return data;
}

std::string decompress(CompressionCodec codec, const std::string &data)
{
    std::string out;
    int err;
    switch (codec)
    {
    case GZIP:
        err = coev::gzip::Decompress(out, data.data(), data.size());
        if (err != 0)
            return data;
        return out;
    case Snappy:
        err = coev::snappy::Decompress(out, data.data(), data.size());
        if (err != 0)
            return data;
        return out;
    case LZ4:
        err = coev::lz4::Decompress(out, data.data(), data.size());
        if (err != 0)
            return data;
        return out;
    case ZSTD:
        err = coev::zstd::Decompress(out, data.data(), data.size());
        if (err != 0)
            return data;
        return out;
    case None:
        return data;
    }
    return data;
}
Message::Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version)
    : m_key(key), m_value(value), m_log_append_time(logAppendTime), m_timestamp(msgTimestamp), m_version(version)
{
}
int Message::encode(packetEncoder &pe)
{
    crc32_field field(CrcCastagnoli);
    pe.push(field);
    defer(pe.pop());

    pe.putInt8(m_version);

    int8_t attributes = static_cast<int8_t>(m_codec) & CompressionCodecMask;
    if (m_log_append_time)
    {
        attributes |= TimestampTypeMask;
    }
    pe.putInt8(attributes);

    if (m_version >= 1)
    {
        auto err = m_timestamp.encode(pe);
        if (err != 0)
        {
            return err;
        }
    }

    int err = pe.putBytes(m_key);
    if (err != 0)
    {
        return err;
    }

    std::string payload;
    if (!m_compressed_cache.empty())
    {
        payload = std::move(m_compressed_cache);
        m_compressed_cache.clear();
    }
    else if (!m_value.empty() || m_codec != CompressionCodec::None)
    {
        payload = compress(m_codec, m_compression_level, m_value);
        if (payload.empty() && !m_value.empty())
        {
            return -1; // compression failed
        }
        m_compressed_cache = payload;
        m_compressed_size = static_cast<int>(payload.size());
    }

    err = pe.putBytes(payload);
    if (err != 0)
    {
        return err;
    }
    return 0;
}

int Message::decode(packetDecoder &pd)
{
    auto field = acquire_crc32_field(CrcCastagnoli);
    int err = pd.push(*field);
    if (err != 0)
        return err;

    defer(pd.pop());
    err = pd.getInt8(m_version);
    if (err != 0)
    {
        return err;
    }

    if (m_version > 1)
    {
        err = -2;
        return err;
    }

    int8_t attribute;
    err = pd.getInt8(attribute);
    if (err != 0)
    {
        return err;
    }

    m_codec = static_cast<CompressionCodec>(attribute & CompressionCodecMask);
    m_log_append_time = (attribute & TimestampTypeMask) != 0;

    if (m_version == 1)
    {
        err = m_timestamp.decode(pd);
        if (err != 0)
        {
            return err;
        }
    }
    err = pd.getBytes(m_key);
    if (err != 0)
    {
        return err;
    }
    err = pd.getBytes(m_value);
    if (err != 0)
    {
        return err;
    }
    m_compressed_size = static_cast<int>(m_value.size());
    if (!m_value.empty() && m_codec != CompressionCodec::None)
    {
        auto decompressed = decompress(m_codec, m_value);
        if (decompressed != m_value)
        {
            m_value = std::move(decompressed);
            err = decode_set();
            if (err != 0)
            {
                return err;
            }
        }
    }
    return 0;
}

int Message::decode_set()
{
    if (m_value.empty())
    {
        m_message_set.clear();
        return 0;
    }

    realDecoder innerDecoder;
    innerDecoder.m_raw.assign(m_value.begin(), m_value.end());
    m_message_set.clear();
    return m_message_set.decode(innerDecoder);
}

void Message::clear()
{
    m_key.clear();
    m_value.clear();
    m_message_set.m_messages.clear();
}