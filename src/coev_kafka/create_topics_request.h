#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct TopicDetail : IEncoder, versioned_decoder
{
    int32_t m_num_partitions;
    int16_t m_replication_factor;
    std::map<int32_t, std::vector<int32_t>> m_replica_assignment;
    std::map<std::string, std::string> m_config_entries;

    TopicDetail() = default;
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct CreateTopicsRequest : protocol_body, flexible_version
{

    int16_t m_version;
    std::map<std::string, std::shared_ptr<TopicDetail>> m_topic_details;
    std::chrono::milliseconds m_timeout;
    bool m_validate_only;

    CreateTopicsRequest() = default;
    CreateTopicsRequest(int16_t v) : m_version(v)
    {
    }
    CreateTopicsRequest(int16_t v, int64_t timeoutMs, bool validateOnly) : m_version(v), m_timeout(timeoutMs), m_validate_only(validateOnly)
    {
    }

    void set_version(int16_t v);

    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};

std::shared_ptr<CreateTopicsRequest> NewCreateTopicsRequest(const KafkaVersion &Version_, std::map<std::string, std::shared_ptr<TopicDetail>> topicDetails, int64_t timeoutMs, bool validateOnly);
