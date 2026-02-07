#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct AlterPartitionReassignmentsBlock : IEncoder, IDecoder
{
    std::vector<int32_t> m_replicas;
    AlterPartitionReassignmentsBlock() = default;
    AlterPartitionReassignmentsBlock(const std::vector<int32_t> &r) : m_replicas(r)
    {
    }
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd);
};

struct AlterPartitionReassignmentsRequest : protocol_body, throttle_support, flexible_version
{
    std::chrono::milliseconds m_timeout;
    std::map<std::string, std::map<int32_t, AlterPartitionReassignmentsBlock>> m_blocks;
    int16_t m_version = 0;
    AlterPartitionReassignmentsRequest() = default;
    AlterPartitionReassignmentsRequest(int16_t v) : m_version(v)
    {
    }
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

    void add_block(const std::string &topic, int32_t partitionID, const std::vector<int32_t> &replicas);
};
