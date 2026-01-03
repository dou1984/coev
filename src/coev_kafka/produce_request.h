#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include "config.h"
#include "protocol_body.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "records.h"
#include "version.h"

struct ProduceRequest : protocolBody
{

    std::string TransactionalID;
    RequiredAcks Acks = RequiredAcks::WaitForLocal;
    std::chrono::milliseconds Timeout;
    int16_t Version = 0;

    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<Records>>> records;

    ProduceRequest() = default;
    ProduceRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;

    void EnsureRecords(const std::string &topic, int32_t partition);
    void AddMessage(const std::string &topic, int32_t partition, std::shared_ptr<Message> msg);
    void AddSet(const std::string &topic, int32_t partition, std::shared_ptr<MessageSet> set);
    void AddBatch(const std::string &topic, int32_t partition, std::shared_ptr<RecordBatch> batch);

    static int64_t UpdateMsgSetMetrics(const std::shared_ptr<MessageSet> msgSet, std::shared_ptr<metrics::Histogram> compressionRatioMetric, std::shared_ptr<metrics::Histogram> topicCompressionRatioMetric);

    static int64_t UpdateBatchMetrics(const std::shared_ptr<RecordBatch> recordBatch, std::shared_ptr<metrics::Histogram> compressionRatioMetric, std::shared_ptr<metrics::Histogram> topicCompressionRatioMetric);
};
