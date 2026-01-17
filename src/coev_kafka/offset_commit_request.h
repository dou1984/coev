#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "protocol_body.h"

struct OffsetCommitRequestBlock : versioned_decoder, versioned_encoder
{
    int64_t m_offset;
    int64_t m_timestamp;
    int32_t m_committed_leader_epoch;
    std::string m_metadata;
    OffsetCommitRequestBlock() = default;
    OffsetCommitRequestBlock(int64_t _offset, int64_t _timestamp, int32_t _epoch, std::string _metadata)
        : m_offset(_offset), m_timestamp(_timestamp), m_committed_leader_epoch(_epoch), m_metadata(_metadata)
    {
    }
    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};

struct OffsetCommitRequest : protocol_body
{
    std::string m_consumer_group;
    int32_t m_consumer_group_generation;
    std::string m_consumer_id;
    std::string m_group_instance_id;
    int64_t m_retention_time;
    int16_t m_version;

    OffsetCommitRequest();
    OffsetCommitRequest(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    void AddBlock(const std::string &topic, int32_t partitionID, int64_t offset, int64_t timestamp, const std::string &metadata);
    void AddBlockWithLeaderEpoch(const std::string &topic, int32_t partitionID, int64_t offset, int32_t leaderEpoch, int64_t timestamp, const std::string &metadata);
    std::pair<int64_t, std::string> Offset(const std::string &topic, int32_t partitionID) const;

    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetCommitRequestBlock>>> blocks;
};
