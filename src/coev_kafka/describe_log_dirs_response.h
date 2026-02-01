#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct DescribeLogDirsResponsePartition
{
    int64_t m_size;
    int64_t m_offset_lag;
    int32_t m_partition_id;
    bool m_is_temporary;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct DescribeLogDirsResponseTopic : versioned_decoder, versioned_encoder
{
    std::string m_topic;
    std::vector<DescribeLogDirsResponsePartition> m_partitions;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct DescribeLogDirsResponseDirMetadata : versioned_decoder, versioned_encoder
{
    KError m_error_code;
    std::string m_path;
    std::vector<DescribeLogDirsResponseTopic> m_topics;
    int64_t m_total_bytes;
    int64_t m_usable_bytes;

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct DescribeLogDirsResponse : protocol_body, flexible_version
{

    std::chrono::milliseconds m_throttle_time;
    int16_t m_version;
    std::vector<DescribeLogDirsResponseDirMetadata> m_log_dirs;
    KError m_error_code;

    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
