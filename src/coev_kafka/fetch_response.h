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
    int64_t ProducerID;
    int64_t FirstOffset;

    AbortedTransaction();
    int decode(PDecoder &pd);
    int encode(PEncoder &pe);
};

struct FetchResponseBlock : VDecoder, VEncoder
{
    KError Err;
    int64_t HighWaterMarkOffset;
    int64_t LastStableOffset;
    int64_t LogStartOffset;
    std::vector<std::shared_ptr<AbortedTransaction>> AbortedTransactions;
    int32_t PreferredReadReplica;
    std::vector<std::shared_ptr<Records>> RecordsSet;

    bool Partial;
    std::shared_ptr<Records> Records_;

    int64_t recordsNextOffset;

    FetchResponseBlock();
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
    int numRecords();
    int isPartial(bool &partial);
    std::vector<std::shared_ptr<AbortedTransaction>> getAbortedTransactions();
};

struct FetchResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    int16_t ErrorCode;
    int32_t SessionID;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<FetchResponseBlock>>> Blocks;

    bool LogAppendTime;
    std::chrono::system_clock::time_point Timestamp;

    FetchResponse();
    void setVersion(int16_t v);
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
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
