#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "records.h"
#include "utils.h"
#include "message.h"
#include "message_set.h"
#include "record_batch.h"
#include "record.h"
#include "errors.h"
#include "control_record.h"
#include "api_versions.h"
#include "protocol_body.h"

struct AbortedTransaction : IEncoder, IDecoder
{
    int64_t m_producer_id;
    int64_t m_first_offset;

    AbortedTransaction();
    int decode(packet_decoder &pd);
    int encode(packet_encoder &pe) const;
};

struct FetchResponseBlock : VDecoder, VEncoder
{
    KError m_err;
    int64_t m_high_water_mark_offset;
    int64_t m_last_stable_offset;
    int64_t m_log_start_offset;
    int32_t m_preferred_read_replica;
    std::vector<AbortedTransaction> m_aborted_transactions;
    std::vector<Records> m_records_set;
    int64_t m_records_next_offset;
    bool m_partial;

    FetchResponseBlock();
    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe, int16_t version) const;
    int num_records() const;
    int is_partial(bool &partial) const;
    std::vector<AbortedTransaction> &get_aborted_transactions();
};

struct FetchResponse : protocol_body
{
    int16_t m_version;
    int16_t m_code;
    int32_t m_session_id;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, std::map<int32_t, FetchResponseBlock>> m_blocks;
    std::chrono::system_clock::time_point m_timestamp;
    bool m_log_append_time;

    FetchResponse();
    void set_version(int16_t v);
    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe) const;
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;

    void add_error(const std::string &topic, int32_t partition, KError err);
    const FetchResponseBlock &get_block(const std::string &topic, int32_t partition) const;
    FetchResponseBlock &get_block(const std::string &topic, int32_t partition);
    FetchResponseBlock &get_or_create_block(const std::string &topic, int32_t partition);

    void add_message_with_timestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                                    std::chrono::system_clock::time_point timestamp, int8_t version);
    void add_record_with_timestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                                   std::chrono::system_clock::time_point timestamp);
    void add_record_batch_with_timestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                                         int64_t producerID, bool isTransactional, std::chrono::system_clock::time_point timestamp);
    void add_control_record_with_timestamp(const std::string &topic, int32_t partition, int64_t offset, int64_t producerID,
                                           ControlRecordType recordType, std::chrono::system_clock::time_point timestamp);
    void add_message(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset);
    void add_record(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset);
    void add_record_batch(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                          int64_t producerID, bool isTransactional);
    void add_control_record(const std::string &topic, int32_t partition, int64_t offset, int64_t producerID, ControlRecordType recordType);
    void set_last_offset_delta(const std::string &topic, int32_t partition, int32_t offset);
    void set_last_stable_offset(const std::string &topic, int32_t partition, int64_t offset);
};
