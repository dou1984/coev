#pragma once

#include <cstdint>
#include <unordered_map>
#include <string>
#include <memory>
#include <chrono>

#include "encoder_decoder.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct ProduceResponseBlock
{

    KError Err;
    int64_t Offset;
    std::chrono::system_clock::time_point Timestamp;
    int64_t StartOffset;

    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe, int16_t version);
};

struct ProduceResponse : protocolBody
{

    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<ProduceResponseBlock>>> Blocks;
    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;

    void setVersion(int16_t v);
    int decode(PDecoder &pd, int16_t version);
    int encode(PEncoder &pe);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
    std::shared_ptr<ProduceResponseBlock> GetBlock(const std::string &topic, int32_t partition) const;
    void AddTopicPartition(const std::string &topic, int32_t partition, KError err);
};
