#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>
#include <string>

#include "record.h"
#include "message.h"
#include "version.h"
#include "config.h"
const int RECORD_BATCH_OVERHEAD = 49;

struct RecordBatch
{
    int8_t m_version = 2;
    CompressionCodec m_codec = CompressionCodec::None;
    int64_t m_first_offset = 0;
    int32_t m_partition_leader_epoch = 0;
    int m_compression_level = 0;
    int32_t m_last_offset_delta = 0;
    std::chrono::system_clock::time_point m_first_timestamp;
    std::chrono::system_clock::time_point m_max_timestamp;
    int64_t m_producer_id = -1;
    int16_t m_producer_epoch = -1;
    int32_t m_first_sequence = -1;
    std::vector<std::shared_ptr<Record>> m_records;
    bool m_control = false;
    bool m_log_append_time = false;
    bool m_partial_trailing_record = false;
    bool m_is_transactional = false;

    std::string m_compressed_records;
    size_t m_records_len = 0;

    RecordBatch() = default;
    RecordBatch(int8_t v);
    RecordBatch(int8_t v, bool, std::chrono::system_clock::time_point &first, std::chrono::system_clock::time_point &max);
    int64_t last_offset() const
    {
        return m_first_offset + static_cast<int64_t>(m_last_offset_delta);
    }
    void add_record(std::shared_ptr<Record> r)
    {
        m_records.emplace_back(std::move(r));
    }

    int encode(packet_encoder &pe);
    int decode(packet_decoder &pd);

    int16_t compute_attributes() const;
    void encode_records(packet_encoder &pe);
};
