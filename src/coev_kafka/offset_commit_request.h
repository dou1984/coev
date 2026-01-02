#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <optional>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "protocol_body.h"

struct OffsetCommitRequestBlock : VDecoder, VEncoder
{
    int64_t Offset;
    int64_t Timestamp;
    int32_t CommittedLeaderEpoch;
    std::string Metadata;
    OffsetCommitRequestBlock() = default;
    OffsetCommitRequestBlock(int64_t _offset, int64_t _timestamp, int32_t _epoch, std::string _metadata)
        : Offset(_offset), Timestamp(_timestamp), CommittedLeaderEpoch(_epoch), Metadata(_metadata)
    {
    }
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct OffsetCommitRequest : protocolBody
{
    std::string ConsumerGroup;
    int32_t ConsumerGroupGeneration;
    std::string ConsumerID;
    std::string GroupInstanceId;
    int64_t RetentionTime;
    int16_t Version;

    OffsetCommitRequest();
    OffsetCommitRequest(int16_t v) : Version(v)
    {
    }

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    void AddBlock(const std::string &topic, int32_t partitionID, int64_t offset, int64_t timestamp, const std::string &metadata);
    void AddBlockWithLeaderEpoch(const std::string &topic, int32_t partitionID, int64_t offset, int32_t leaderEpoch, int64_t timestamp, const std::string &metadata);
    std::pair<int64_t, std::string> Offset(const std::string &topic, int32_t partitionID) const;

    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetCommitRequestBlock>>> blocks;
};
