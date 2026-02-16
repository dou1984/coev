#include <limits>
#include "record_batch.h"
#include "record.h"
#include "message.h"
#include "crc32_field.h"
#include "length_field.h"
#include "timestamp.h"
#include "errors.h"
#include "real_decoder.h"
#include "packet_error.h"

int decode(const std::string &buf, std::vector<Record> &inputs)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    real_decoder helper(buf);
    for (auto &in : inputs)
    {
        if (in.decode(helper) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    if (helper.m_offset != static_cast<int>(buf.size()))
    {
        throw PacketError{"invalid length: buf=" + std::to_string(buf.size()) + " decoded=" + std::to_string(helper.m_offset)};
    }
    return ErrNoError;
}

int decode(const std::string &buf, std::vector<std::shared_ptr<Record>> &inputs)
{
    if (buf.empty())
    {
        return ErrNoError;
    }

    real_decoder helper(buf);
    for (auto i = 0; i < inputs.size(); ++i)
    {
        if (inputs[i] == nullptr)
        {
            inputs[i] = std::make_shared<Record>();
        }
        if (inputs[i]->decode(helper) != ErrNoError)
        {
            return ErrDecodeError;
        }
    }
    if (helper.m_offset != static_cast<int>(buf.size()))
    {
        throw PacketError{"invalid length: buf=" + std::to_string(buf.size()) + " decoded=" + std::to_string(helper.m_offset)};
    }
    return ErrNoError;
}
RecordBatch::RecordBatch(int8_t v) : m_version(v)
{
}
RecordBatch::RecordBatch(int8_t v, bool, std::chrono::system_clock::time_point &first, std::chrono::system_clock::time_point &max) : m_version(v), m_first_timestamp(first), m_max_timestamp(max)
{
}
int RecordBatch::encode(packet_encoder &pe) const
{
    if (m_version != 2)
    {
        return ErrUnsupportedRecordBatchVersion;
    }

    pe.putInt64(m_first_offset);

    LengthField length_field;
    pe.push(length_field);

    pe.putInt32(m_partition_leader_epoch);
    pe.putInt8(m_version);

    crc32_field cf(CrcPolynomial::CrcCastagnoli);
    pe.push(cf);

    pe.putInt16(compute_attributes());
    pe.putInt32(m_last_offset_delta);

    Timestamp first = m_first_timestamp;
    first.encode(pe);

    Timestamp max_time = m_max_timestamp;
    max_time.encode(pe);

    pe.putInt64(m_producer_id);
    pe.putInt16(m_producer_epoch);
    pe.putInt32(m_first_sequence);

    pe.putArrayLength(static_cast<int>(m_records.size()));

    if (!m_compressed_records.empty())
    {
        pe.putRawBytes(m_compressed_records);
    }
    else
    {
        std::string raw;
        for (auto record : m_records)
        {
            assert(record);
            ::encode(*record, raw);
        }
        std::string compressed;
        auto err = compress(m_codec, m_compression_level, raw, compressed);
        pe.putRawBytes(compressed);
    }

    pe.pop();
    pe.pop();
    return 0;
}

int RecordBatch::decode(packet_decoder &pd)
{
    int err = pd.getInt64(m_first_offset);
    if (err)
    {
        return err;
    }

    int32_t batch_len;
    err = pd.getInt32(batch_len);
    if (err)
    {
        return err;
    }
    err = pd.getInt32(m_partition_leader_epoch);
    if (err)
    {
        return err;
    }
    err = pd.getInt8(m_version);
    if (err)
    {
        return err;
    }

    if (m_version != 2)
    {
        throw std::runtime_error("unsupported record batch version (" + std::to_string(m_version) + ")");
    }

    crc32_field cf(CrcPolynomial::CrcCastagnoli);
    pd.push(cf);

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

    int num_record;
    pd.getArrayLength(num_record);
    if (num_record >= 0)
    {
        m_records.resize(num_record);
    }

    int buf_size = static_cast<int>(batch_len) - RECORD_BATCH_OVERHEAD;
    std::string_view view;
    err = pd.getRawBytes(buf_size, view);
    if (err != ErrNoError)
    {
        return err;
    }
    std::string rec_buffer(view.data(), view.size());

    pd.pop();
    std::string decompressed;
    err = ::decompress(m_codec, rec_buffer, decompressed);
    m_records_len = decompressed.size();

    try
    {
        int offset = 0;
        for (auto i = 0; i < num_record; ++i)
        {
            ::decode(decompressed, m_records);
            offset += m_records[i]->m_length.m_length;
        }
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

int16_t RecordBatch::compute_attributes() const
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

void RecordBatch::encode_records(packet_encoder &pe)
{

    std::string raw;
    for (auto record : m_records)
    {
        assert(record);
        ::encode(*record, raw);
    }

    m_records_len = raw.size();
    compress(m_codec, m_compression_level, raw, m_compressed_records);
}
int64_t RecordBatch::last_offset() const
{
    return m_first_offset + static_cast<int64_t>(m_last_offset_delta);
}
void RecordBatch::add_record(std::shared_ptr<Record> record)
{
    m_records.push_back(record);
}