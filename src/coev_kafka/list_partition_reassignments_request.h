#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct ListPartitionReassignmentsRequest : protocol_body, flexible_version
{
    std::chrono::milliseconds m_timeout;
    std::unordered_map<std::string, std::vector<int32_t>> m_blocks;
    int16_t m_version = 0;
    ListPartitionReassignmentsRequest() = default;
    ListPartitionReassignmentsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t ver) const;
    KafkaVersion required_version() const;

    void add_block(const std::string &topic, const std::vector<int32_t> &partitionIDs);
};
