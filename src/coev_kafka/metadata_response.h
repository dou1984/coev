#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "errors.h"
#include "uuid.h"
#include "protocol_body.h"

struct Broker;

struct PartitionMetadata : VDecoder, VEncoder
{
    mutable int16_t m_version;
    KError m_err;
    int32_t m_id;
    int32_t m_leader;
    int32_t m_leader_epoch;
    std::vector<int32_t> m_replicas;
    std::vector<int32_t> m_isr;
    std::vector<int32_t> m_offline_replicas;

    PartitionMetadata() = default;
    PartitionMetadata(int16_t v) : m_version(v)
    {
    }
    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe, int16_t version) const;
};

struct TopicMetadata : VDecoder, VEncoder
{
    mutable int16_t m_version;
    KError m_err;
    std::string m_name;
    Uuid m_uuid;
    bool m_is_internal;
    std::vector<PartitionMetadata> m_partitions;
    int32_t m_topic_authorized_operations;

    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe, int16_t version) const;
};

struct MetadataResponse : protocol_body, flexible_version
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<std::shared_ptr<Broker>> m_brokers;
    std::vector<std::shared_ptr<TopicMetadata>> m_topics;
    std::string m_cluster_id;
    int32_t m_controller_id;
    int32_t m_cluster_authorized_operations;

    void set_version(int16_t v);
    int decode(packet_decoder &pd, int16_t version);
    int encode(packet_encoder &pe) const;
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;

    void add_broker(const std::string &addr, int32_t id);
    std::shared_ptr<TopicMetadata> add_topic(const std::string &topic, int16_t err);
    int add_topic_partition(
        const std::string &topic, int32_t partition, int32_t brokerID, const std::vector<int32_t> &replicas, const std::vector<int32_t> &isr,
        const std::vector<int32_t> &offline, int err);
};
