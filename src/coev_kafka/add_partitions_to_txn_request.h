#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "protocol_body.h"

struct AddPartitionsToTxnRequest : protocol_body
{
    int16_t m_version;
    std::string m_transactional_id;
    int64_t m_producer_id;
    int16_t m_producer_epoch;
    std::unordered_map<std::string, std::vector<int32_t>> m_topic_partitions;
    AddPartitionsToTxnRequest() = default;
    AddPartitionsToTxnRequest(int16_t v) : m_version(v)
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
};
