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
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct AlterPartitionReassignmentsRequest : protocol_body, throttle_support
{
    std::chrono::milliseconds m_timeout;
    std::map<std::string, std::map<int32_t, std::shared_ptr<AlterPartitionReassignmentsBlock>>> m_blocks;
    int16_t m_version = 0;
    AlterPartitionReassignmentsRequest() = default;
    AlterPartitionReassignmentsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;

    void AddBlock(const std::string &topic, int32_t partitionID, const std::vector<int32_t> &replicas);
};
