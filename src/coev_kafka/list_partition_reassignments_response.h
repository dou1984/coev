#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct PartitionReplicaReassignmentsStatus
{
    std::vector<int32_t> m_replicas;
    std::vector<int32_t> m_adding_replicas;
    std::vector<int32_t> m_removing_replicas;

    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd);
};

struct ListPartitionReassignmentsResponse : protocol_body, flexible_version
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_err;
    std::string m_error_message;
    std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionReplicaReassignmentsStatus>>> m_topic_status;

    void set_version(int16_t v);
    void AddBlock(const std::string &topic, int32_t partition,
                  const std::vector<int32_t> &replicas,
                  const std::vector<int32_t> &addingReplicas,
                  const std::vector<int32_t> &removingReplicas);

    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t ver) const;
    KafkaVersion required_version() const;

    std::chrono::milliseconds throttle_time() const;
};
