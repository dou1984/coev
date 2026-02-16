#include <cstring>
#include <map>
#include <utils/hash/crc32.h>
#include "message.h"
#include "message_set.h"
#include "timestamp.h"
#include "real_decoder.h"
#include "undefined.h"
#include "crc32_field.h"

std::string ToString(CompressionCodec codec)
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

bool FromString(const std::string &s, CompressionCodec &out)
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

int compress(CompressionCodec codec, int level, std::string_view data, std::string &out)
{

    switch (codec)
    {
    case GZIP:
        return coev::gzip::Compress(out, data.data(), data.size());
    case Snappy:
        return coev::snappy::Compress(out, data.data(), data.size());

    case LZ4:
        return coev::lz4::Compress(out, data.data(), data.size());

    case ZSTD:
        return coev::zstd::Compress(out, data.data(), data.size());

    default:
        out.assign(data.data(), data.size());
        return ErrNoError;
    }
}

int decompress(CompressionCodec codec, std::string_view data, std::string &out)
{
    switch (codec)
    {
    case GZIP:
        return coev::gzip::Decompress(out, data.data(), data.size());
    case Snappy:
        return coev::snappy::Decompress(out, data.data(), data.size());
    case LZ4:
        return coev::lz4::Decompress(out, data.data(), data.size());
    case ZSTD:
        return coev::zstd::Decompress(out, data.data(), data.size());
    case None:
    default:
        out.assign(data.data(), data.size());
        return ErrNoError;
    }
}
Message::Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version)
    : m_key(key), m_value(value), m_log_append_time(logAppendTime), m_timestamp(msgTimestamp), m_version(version)
{
}
int Message::encode(packet_encoder &pe) const
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
        err = compress(m_codec, m_compression_level, m_value, payload);
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

int Message::decode(packet_decoder &pd)
{
    auto field = acquire_crc32_field(CrcCastagnoli);
    int err = pd.push(*field);
    if (err != 0)
        return err;

    defer(pd.pop());
    err = pd.getInt8(m_version);
    if (err != 0)
    {
        LOG_CORE("Message::decode %d", err);
        return err;
    }

    if (m_version > 1)
    {
        err = -2;
        LOG_CORE("Message::decode %d", err);
        return err;
    }

    int8_t attribute;
    err = pd.getInt8(attribute);
    if (err != 0)
    {
        LOG_CORE("Message::decode %d", err);
        return err;
    }

    m_codec = static_cast<CompressionCodec>(attribute & CompressionCodecMask);
    m_log_append_time = (attribute & TimestampTypeMask) != 0;

    if (m_version == 1)
    {
        err = m_timestamp.decode(pd);
        if (err != 0)
        {
            LOG_CORE("Message::decode %d", err);
            return err;
        }
    }
    err = pd.getBytes(m_key);
    if (err != 0)
    {
        LOG_CORE("Message::decode %d", err);
        return err;
    }
    err = pd.getBytes(m_value);
    if (err != 0)
    {
        LOG_CORE("Message::decode %d", err);
        return err;
    }
    m_compressed_size = static_cast<int>(m_value.size());
    if (!m_value.empty() && m_codec != CompressionCodec::None)
    {
        std::string decompressed;
        err = ::decompress(m_codec, m_value, decompressed);
        if (decompressed != m_value)
        {
            m_value = std::move(decompressed);
            err = decode_set();
            if (err != 0)
            {
                LOG_CORE("Message::decode %d", err);
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

    real_decoder inner_decoder(m_value);
    m_message_set.clear();
    return m_message_set.decode(inner_decoder);
}

void Message::clear()
{
    m_key.clear();
    m_value.clear();
    m_message_set.m_messages.clear();
}