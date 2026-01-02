#include <cstring>
#include <map>
#include "message.h"
#include "message_set.h"
#include "timestamp.h"
#include "compress.h"
#include "real_decoder.h"
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

std::string compress(CompressionCodec /*codec*/, int /*level*/, const std::string &data)
{
    return data; // dummy
}

std::string decompress(CompressionCodec /*codec*/, const std::string &data)
{
    return data; // dummy
}
Message::Message(const std::string &key, const std::string &value, bool logAppendTime, Timestamp msgTimestamp, int8_t version)
    : Key(key), Value(value), LogAppendTime(logAppendTime), Timestamp_(msgTimestamp), Version(version)
{
}
int Message::encode(PEncoder &pe)
{

    pe.push(std::dynamic_pointer_cast<pushEncoder>(std::make_shared<CRC32>((int)CrcPolynomial::CrcIEEE)));

    pe.putInt8(Version);

    int8_t attributes = static_cast<int8_t>(Codec) & CompressionCodecMask;
    if (LogAppendTime)
    {
        attributes |= TimestampTypeMask;
    }
    pe.putInt8(attributes);

    if (Version >= 1)
    {

        auto err = Timestamp_.encode(pe);
        if (err != 0)
        {
            pe.pop();
            return err;
        }
    }

    int err = pe.putBytes(Key);
    if (err != 0)
    {
        pe.pop();
        return err;
    }

    std::string payload;

    if (!compressedCache.empty())
    {
        payload = std::move(compressedCache);
        compressedCache.clear();
    }
    else if (!Value.empty())
    {
        payload = compress(Codec, CompressionLevel, Value);
        if (payload.empty() && !Value.empty())
        {
            pe.pop();
            return -1; // compression failed
        }
        compressedCache = payload;
        compressedSize = static_cast<int>(payload.size());
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

    err = pd.getInt8(Version);
    if (err != 0)
        goto pop_and_return;

    if (Version > 1)
    {
        err = -2;
        goto pop_and_return;
    }

    int8_t attribute;
    err = pd.getInt8(attribute);
    if (err != 0)
        goto pop_and_return;

    Codec = static_cast<CompressionCodec>(attribute & CompressionCodecMask);
    LogAppendTime = (attribute & TimestampTypeMask) != 0;

    if (Version == 1)
    {

        err = Timestamp_.decode(pd);
        if (err != 0)
            goto pop_and_return;
    }

    err = pd.getBytes(Key);
    if (err != 0)
        goto pop_and_return;

    err = pd.getBytes(Value);
    if (err != 0)
        goto pop_and_return;

    compressedSize = static_cast<int>(Value.size());

    if (!Value.empty() && Codec != CompressionCodec::None)
    {
        auto decompressed = decompress(Codec, Value);
        if (decompressed.empty())
        {
            err = -3; // decompression failed
            goto pop_and_return;
        }
        Value = std::move(decompressed);

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
    innerDecoder.Raw.assign(Value.begin(), Value.end());
    Set = std::make_shared<MessageSet>();
    return Set->decode(innerDecoder);
}