#include <cstring>
#include <map>
#include "message.h"
#include "message_set.h"
#include "timestamp.h"
#include "compress.h"
#include "real_decoder.h"
#include "../utils/compress/coev_compress.h"
#include "undefined.h"
#include "crc32_field.h"
#include "crc32.h"
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
        data;
    }
    return data;
}

std::string decompress(CompressionCodec codec, const std::string &data)
{
    std::string out;
    switch (codec)
    {
    case GZIP:
        coev::gzip::Decompress(out, data.data(), data.size());
        return out;
    case Snappy:
        coev::snappy::Decompress(out, data.data(), data.size());
        return out;
    case LZ4:
        coev::lz4::Decompress(out, data.data(), data.size());
        return out;
    case ZSTD:
        coev::zstd::Decompress(out, data.data(), data.size());
        return out;
    case None:
        return data;
    }
}
Message::Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version)
    : m_key(key), m_value(value), m_log_append_time(logAppendTime), m_timestamp(msgTimestamp), m_version(version)
{
}
int Message::encode(PEncoder &pe)
{

    pe.push(std::dynamic_pointer_cast<pushEncoder>(std::make_shared<CRC32>((int)CrcPolynomial::CrcIEEE)));

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
            pe.pop();
            return err;
        }
    }

    int err = pe.putBytes(m_key);
    if (err != 0)
    {
        pe.pop();
        return err;
    }

    std::string payload;

    if (!m_compressed_cache.empty())
    {
        payload = std::move(m_compressed_cache);
        m_compressed_cache.clear();
    }
    else if (!m_value.empty())
    {
        payload = compress(m_codec, m_compression_level, m_value);
        if (payload.empty() && !m_value.empty())
        {
            pe.pop();
            return -1; // compression failed
        }
        m_compressed_cache = payload;
        m_compressed_size = static_cast<int>(payload.size());
    }

    err = pe.putBytes(payload);
    if (err != 0)
    {
        pe.pop();
        return err;
    }

    return pe.pop();
}

int Message::decode(PDecoder &pd)
{
    int err = pd.push(std::make_shared<CRC32>(CrcPolynomial::CrcIEEE));
    if (err != 0)
        return err;

    err = pd.getInt8(m_version);
    if (err != 0)
        goto pop_and_return;

    if (m_version > 1)
    {
        err = -2;
        goto pop_and_return;
    }

    int8_t attribute;
    err = pd.getInt8(attribute);
    if (err != 0)
        goto pop_and_return;

    m_codec = static_cast<CompressionCodec>(attribute & CompressionCodecMask);
    m_log_append_time = (attribute & TimestampTypeMask) != 0;

    if (m_version == 1)
    {

        err = m_timestamp.decode(pd);
        if (err != 0)
            goto pop_and_return;
    }

    err = pd.getBytes(m_key);
    if (err != 0)
        goto pop_and_return;

    err = pd.getBytes(m_value);
    if (err != 0)
        goto pop_and_return;

    m_compressed_size = static_cast<int>(m_value.size());
    if (!m_value.empty() && m_codec != CompressionCodec::None)
    {
        auto decompressed = decompress(m_codec, m_value);
        if (decompressed.empty())
        {
            err = -3; // decompression failed
            goto pop_and_return;
        }
        m_value = std::move(decompressed);

        err = decodeSet();
        if (err != 0)
            goto pop_and_return;
    }

pop_and_return:
    int popErr = pd.pop();
    return err != 0 ? err : popErr;
}

int Message::decodeSet()
{
    realDecoder innerDecoder;
    innerDecoder.m_raw.assign(m_value.begin(), m_value.end());
    m_set = std::make_shared<MessageSet>();
    return m_set->decode(innerDecoder);
}