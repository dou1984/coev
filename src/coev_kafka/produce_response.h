#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <memory>
#include <chrono>

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
    std::chrono::system_clock::time_point m_timestamp;
    int64_t m_start_offset = 0;

    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
};

struct ProduceResponse : protocol_body
{

    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<ProduceResponseBlock>>> m_blocks;
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;

    void set_version(int16_t v);
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    std::shared_ptr<ProduceResponseBlock> GetBlock(const std::string &topic, int32_t partition) const;
    void AddTopicPartition(const std::string &topic, int32_t partition, KError err);
};
