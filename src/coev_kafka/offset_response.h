#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "errors.h"
#include "api_versions.h"
#include "protocol_body.h"

struct OffsetResponseBlock : VDecoder, VEncoder
{
    KError m_err;
    std::vector<int64_t> m_offsets;
    int64_t m_timestamp;
    int64_t m_offset;
    int32_t m_leader_epoch;

    OffsetResponseBlock();

    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe, int16_t version) const;
};

struct OffsetResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::unordered_map<std::string, std::unordered_map<int32_t, OffsetResponseBlock>> m_blocks;

    void set_version(int16_t v);
    int decode(packet_decoder &pd, int16_t version);
    OffsetResponseBlock &get_block(const std::string &topic, int32_t partition);
    bool has_block(const std::string &topic, int32_t partition);
    int encode(packet_encoder &pe) const;
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    void add_topic_partition(const std::string &topic, int32_t partition, int64_t offset);
};
