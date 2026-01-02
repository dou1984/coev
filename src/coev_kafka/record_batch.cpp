#include <limits>
#include "record_batch.h"
#include "record.h"
#include "compress.h"
#include "crc32_field.h"
#include "length_field.h"
#include "timestamp.h"
#include "errors.h"

RecordBatch::RecordBatch(int8_t v) : Version(v)
{
}
RecordBatch::RecordBatch(int8_t v, bool, std::chrono::system_clock::time_point &first, std::chrono::system_clock::time_point &max) : Version(v), FirstTimestamp(first), MaxTimestamp(max)
{
}
int RecordBatch::encode(PEncoder &pe)
{
    if (Version != 2)
    {

        return ErrUnsupportedRecordBatchVersion;
    }

    pe.putInt64(FirstOffset);

    auto lenField = std::make_shared<LengthField>();
    pe.push(lenField);

    pe.putInt32(PartitionLeaderEpoch);
    pe.putInt8(Version);

    auto crcField = std::make_shared<crc32Field>(CrcPolynomial::CrcCastagnoli);
    pe.push(crcField);

    pe.putInt16(ComputeAttributes());
    pe.putInt32(LastOffsetDelta);

    Timestamp first = FirstTimestamp;
    first.encode(pe);

    Timestamp max_time = MaxTimestamp;
    max_time.encode(pe);

    pe.putInt64(ProducerID);
    pe.putInt16(ProducerEpoch);
    pe.putInt32(FirstSequence);

    pe.putArrayLength(static_cast<int>(Records.size()));

    if (compressedRecords.empty())
    {
        EncodeRecords(pe);
    }

    pe.putRawBytes(compressedRecords);

    pe.pop();
    pe.pop();
    return 0;
}

int RecordBatch::decode(PDecoder &pd)
{
    int err = pd.getInt64(FirstOffset);
    if (err)
        return err;

    int32_t batchLen;
    err = pd.getInt32(batchLen);
    if (err)
        return err;
    err = pd.getInt32(PartitionLeaderEpoch);
    if (err)
        return err;
    err = pd.getInt8(Version);
    if (err)
        return err;

    if (Version != 2)
    {
        throw std::runtime_error("unsupported record batch version (" + std::to_string(Version) + ")");
    }

    auto crcField = std::make_shared<crc32Field>(CrcPolynomial::CrcCastagnoli);
    pd.push(std::dynamic_pointer_cast<pushDecoder>(crcField));

    int16_t attributes;
    pd.getInt16(attributes);
    Codec = static_cast<CompressionCodec>(attributes & 0x07);
    Control = (attributes & 0x20) != 0;
    LogAppendTime = (attributes & 0x08) != 0;
    IsTransactional = (attributes & 0x10) != 0;

    pd.getInt32(LastOffsetDelta);

    Timestamp first = FirstTimestamp;
    first.decode(pd);
    Timestamp max_time = MaxTimestamp;
    max_time.decode(pd);

    pd.getInt64(ProducerID);
    pd.getInt16(ProducerEpoch);
    pd.getInt32(FirstSequence);

    int numRecs;
    pd.getArrayLength(numRecs);
    if (numRecs >= 0)
    {
        Records.resize(numRecs);
        for (auto &r : Records)
            r = std::make_unique<Record>();
    }

    int bufSize = static_cast<int>(batchLen) - RECORD_BATCH_OVERHEAD;
    std::string recBuffer;
    try
    {
        pd.getRawBytes(bufSize, recBuffer);
    }
    catch (const std::runtime_error &e)
    {
        if (std::string(e.what()).find("InsufficientData") != std::string::npos)
        {
            PartialTrailingRecord = true;
            Records.clear();
            pd.pop();
        }
    }

    pd.pop();
    auto decompressed = decompress(Codec, recBuffer);
    recordsLen = decompressed.size();

    try
    {
        auto record = std::make_shared<Record>();
        int l = ::decode(decompressed, std::dynamic_pointer_cast<IDecoder>(record), nullptr);
        Records.push_back(record);
    }
    catch (const std::runtime_error &e)
    {
        if (std::string(e.what()).find("InsufficientData") != std::string::npos)
        {
            PartialTrailingRecord = true;
            Records.clear();
        }
    }
    return 0;
}

int16_t RecordBatch::ComputeAttributes() const
{
    int16_t attr = static_cast<int16_t>(static_cast<int8_t>(Codec)) & 0x07;
    if (Control)
        attr |= 0x20;
    if (LogAppendTime)
        attr |= 0x08;
    if (IsTransactional)
        attr |= 0x10;
    return attr;
}

void RecordBatch::EncodeRecords(PEncoder &pe)
{

    std::string raw;
    for (auto &record : Records)
    {
        ::encode(record, raw, pe.metricRegistry());
    }

    recordsLen = raw.size();
    compressedRecords = compress(Codec, CompressionLevel, raw);
}