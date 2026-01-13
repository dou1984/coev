#include <limits>
#include "record_batch.h"
#include "record.h"
#include "message.h"
#include "crc32_field.h"
#include "length_field.h"
#include "timestamp.h"
#include "errors.h"

RecordBatch::RecordBatch(int8_t v) : m_version(v)
{
}
RecordBatch::RecordBatch(int8_t v, bool, std::chrono::system_clock::time_point &first, std::chrono::system_clock::time_point &max) : m_version(v), m_first_timestamp(first), m_max_timestamp(max)
{
}
int RecordBatch::encode(packetEncoder &pe)
{
    if (m_version != 2)
    {

        return ErrUnsupportedRecordBatchVersion;
    }

    pe.putInt64(m_first_offset);

    auto lenField = std::make_shared<LengthField>();
    pe.push(lenField);

    pe.putInt32(m_partition_leader_epoch);
    pe.putInt8(m_version);

    auto crcField = std::make_shared<crc32_field>(CrcPolynomial::CrcCastagnoli);
    pe.push(crcField);

    pe.putInt16(ComputeAttributes());
    pe.putInt32(m_last_offset_delta);

    Timestamp first = m_first_timestamp;
    first.encode(pe);

    Timestamp max_time = m_max_timestamp;
    max_time.encode(pe);

    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);
    pe.putInt32(m_first_sequence);

    pe.putArrayLength(static_cast<int>(m_records.size()));

    if (m_compressed_records.empty())
    {
        EncodeRecords(pe);
    }

    pe.putRawBytes(m_compressed_records);

    pe.pop();
    pe.pop();
    return 0;
}

int RecordBatch::decode(packetDecoder &pd)
{
    int err = pd.getInt64(m_first_offset);
    if (err)
        return err;

    int32_t batchLen;
    err = pd.getInt32(batchLen);
    if (err)
        return err;
    err = pd.getInt32(m_partition_leader_epoch);
    if (err)
        return err;
    err = pd.getInt8(m_version);
    if (err)
        return err;

    if (m_version != 2)
    {
        throw std::runtime_error("unsupported record batch version (" + std::to_string(m_version) + ")");
    }

    auto crcField = std::make_shared<crc32_field>(CrcPolynomial::CrcCastagnoli);
    pd.push(std::dynamic_pointer_cast<pushDecoder>(crcField));

    int16_t attributes;
    pd.getInt16(attributes);
    m_codec = static_cast<CompressionCodec>(attributes & 0x07);
    m_control = (attributes & 0x20) != 0;
    m_log_append_time = (attributes & 0x08) != 0;
    m_is_transactional = (attributes & 0x10) != 0;

    pd.getInt32(m_last_offset_delta);

    Timestamp first = m_first_timestamp;
    first.decode(pd);
    Timestamp max_time = m_max_timestamp;
    max_time.decode(pd);

    pd.getInt64(m_producer_id);
    pd.getInt16(m_producer_epoch);
    pd.getInt32(m_first_sequence);

    int numRecs;
    pd.getArrayLength(numRecs);
    if (numRecs >= 0)
    {
        m_records.resize(numRecs);
        for (auto &r : m_records)
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
            m_partial_trailing_record = true;
            m_records.clear();
            pd.pop();
        }
    }

    pd.pop();
    auto decompressed = decompress(m_codec, recBuffer);
    m_records_len = decompressed.size();

    try
    {
        auto record = std::make_shared<Record>();
        int l = ::decode(decompressed, std::dynamic_pointer_cast<IDecoder>(record), nullptr);
        m_records.push_back(record);
    }
    catch (const std::runtime_error &e)
    {
        if (std::string(e.what()).find("InsufficientData") != std::string::npos)
        {
            m_partial_trailing_record = true;
            m_records.clear();
        }
    }
    return 0;
}

int16_t RecordBatch::ComputeAttributes() const
{
    int16_t attr = static_cast<int16_t>(static_cast<int8_t>(m_codec)) & 0x07;
    if (m_control)
        attr |= 0x20;
    if (m_log_append_time)
        attr |= 0x08;
    if (m_is_transactional)
        attr |= 0x10;
    return attr;
}

void RecordBatch::EncodeRecords(packetEncoder &pe)
{

    std::string raw;
    for (auto &record : m_records)
    {
        ::encode(record, raw, pe.metricRegistry());
    }

    m_records_len = raw.size();
    m_compressed_records = compress(m_codec, m_compression_level, raw);
}