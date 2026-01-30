#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <memory>
#include <chrono>
#include <map>
#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct ProduceResponseBlock
{
    KError m_err;
    int64_t m_offset = 0;
    int64_t m_start_offset = 0;
    std::chrono::system_clock::time_point m_timestamp;

    int decode(packetDecoder & pd, int16_t version);
    int encode(packetEncoder & pe, int16_t version) const;
};

struct ProduceResponse : protocol_body
{
    std::unordered_map<std::string, std::map<int32_t, ProduceResponseBlock>> m_blocks;
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;

    void set_version(int16_t v);
    int decode(packetDecoder &pd, int16_t version);
    int encode(packetEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    ProduceResponseBlock &get_block(const std::string &topic, int32_t partition);
    void add_topic_partition(const std::string &topic, int32_t partition, KError err);
};
