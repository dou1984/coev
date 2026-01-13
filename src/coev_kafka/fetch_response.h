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
    int decode(packetDecoder &pd);
    int encode(packetEncoder &pe);
};

struct FetchResponseBlock : versionedDecoder, versionedEncoder
{
    KError m_err;
    int64_t m_high_water_mark_offset;
    int64_t m_last_stable_offset;
    int64_t m_log_start_offset;
    std::vector<std::shared_ptr<AbortedTransaction>> m_aborted_transactions;
    int32_t m_preferred_read_replica;
    std::vector<std::shared_ptr<Records>> m_records_set;
    std::shared_ptr<Records> m_records;
    int64_t m_records_next_offset;
    bool m_partial;

    FetchResponseBlock();
    int decode(packetDecoder &pd, int16_t version);
    int encode(packetEncoder &pe, int16_t version);
    int numRecords();
    int isPartial(bool &partial);
    std::vector<std::shared_ptr<AbortedTransaction>> getAbortedTransactions();
};

struct FetchResponse : protocol_body
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    int16_t ErrorCode;
    int32_t SessionID;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<FetchResponseBlock>>> m_blocks;

    bool LogAppendTime;
    std::chrono::system_clock::time_point Timestamp;

    FetchResponse();
    void set_version(int16_t v);
    int decode(packetDecoder &pd, int16_t version);
    int encode(packetEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    std::shared_ptr<FetchResponseBlock> GetBlock(const std::string &topic, int32_t partition);
    void AddError(const std::string &topic, int32_t partition, KError err);
    std::shared_ptr<FetchResponseBlock> getOrCreateBlock(const std::string &topic, int32_t partition);

    void AddMessageWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                                 std::chrono::system_clock::time_point timestamp, int8_t version);
    void AddRecordWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                                std::chrono::system_clock::time_point timestamp);
    void AddRecordBatchWithTimestamp(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                                     int64_t producerID, bool isTransactional, std::chrono::system_clock::time_point timestamp);
    void AddControlRecordWithTimestamp(const std::string &topic, int32_t partition, int64_t offset, int64_t producerID,
                                       ControlRecordType recordType, std::chrono::system_clock::time_point timestamp);
    void AddMessage(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset);
    void AddRecord(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset);
    void AddRecordBatch(const std::string &topic, int32_t partition, Encoder *key, Encoder *value, int64_t offset,
                        int64_t producerID, bool isTransactional);
    void AddControlRecord(const std::string &topic, int32_t partition, int64_t offset, int64_t producerID, ControlRecordType recordType);
    void SetLastOffsetDelta(const std::string &topic, int32_t partition, int32_t offset);
    void SetLastStableOffset(const std::string &topic, int32_t partition, int64_t offset);
};
