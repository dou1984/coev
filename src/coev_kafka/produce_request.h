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

struct ProduceRequest : protocol_body
{

    std::string m_transactional_id;
    RequiredAcks m_acks = RequiredAcks::WaitForLocal;
    std::chrono::milliseconds m_timeout;
    int16_t m_version = 0;

    std::unordered_map<std::string, std::map<int32_t, Records>> m_records;

    ProduceRequest() = default;
    ProduceRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;

    void ensure_records(const std::string &topic, int32_t partition);
    void add_message(const std::string &topic, int32_t partition, std::shared_ptr<Message> msg);
    void add_set(const std::string &topic, int32_t partition, MessageSet &set);
    void add_batch(const std::string &topic, int32_t partition, RecordBatch &batch);
};
