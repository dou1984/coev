#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "partition_error.h"

struct AddPartitionsToTxnResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::unordered_map<std::string, std::vector<std::shared_ptr<PartitionError>>> m_errors;

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
