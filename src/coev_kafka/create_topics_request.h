#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct TopicDetail : IEncoder, VDecoder
{
    int32_t m_num_partitions;
    int16_t m_replication_factor;
    std::map<int32_t, std::vector<int32_t>> m_replica_assignment;
    std::map<std::string, std::string> m_config_entries;

    TopicDetail() = default;
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
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

    CreateTopicsRequest(const KafkaVersion &version, int64_t timeoutMs, bool validateOnly, std::map<std::string, std::shared_ptr<TopicDetail>> &topicDetails);

    void set_version(int16_t v);

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
