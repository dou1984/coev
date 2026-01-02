#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "errors.h"
#include "api_versions.h"
#include "protocol_body.h"

struct OffsetResponseBlock
{
    KError Err;
    std::vector<int64_t> Offsets;
    int64_t Timestamp;
    int64_t Offset;
    int32_t LeaderEpoch;

    OffsetResponseBlock();

    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
};

struct OffsetResponse : protocolBody
{

    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetResponseBlock>>> Blocks_;

    void setVersion(int16_t v);
    int decode(PDecoder &pd, int16_t version);
    std::shared_ptr<OffsetResponseBlock> GetBlock(const std::string &topic, int32_t partition);
    int encode(PEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
    void AddTopicPartition(const std::string &topic, int32_t partition, int64_t offset);
};
