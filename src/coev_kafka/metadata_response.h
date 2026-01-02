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
    int16_t Version;
    KError Err;
    int32_t ID;
    int32_t Leader;
    int32_t LeaderEpoch;
    std::vector<int32_t> Replicas;
    std::vector<int32_t> Isr;
    std::vector<int32_t> OfflineReplicas;

    PartitionMetadata() = default;
    PartitionMetadata(int16_t v) : Version(v)
    {
    }
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
};

struct TopicMetadata : VDecoder, VEncoder
{
    int16_t Version;
    KError Err;
    std::string Name;
    Uuid Uuid_;
    bool IsInternal;
    std::vector<std::shared_ptr<PartitionMetadata>> Partitions;
    int32_t TopicAuthorizedOperations;

    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
};

struct MetadataResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::vector<std::shared_ptr<Broker>> Brokers;
    std::string ClusterID;
    int32_t ControllerID;
    std::vector<std::shared_ptr<TopicMetadata>> Topics;
    int32_t ClusterAuthorizedOperations;

    void setVersion(int16_t v);
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;

    void AddBroker(const std::string &addr, int32_t id);
    std::shared_ptr<TopicMetadata> AddTopic(const std::string &topic, int16_t err);
    int AddTopicPartition(
        const std::string &topic, int32_t partition, int32_t brokerID, const std::vector<int32_t> &replicas, const std::vector<int32_t> &isr,
        const std::vector<int32_t> &offline, int err);
};
