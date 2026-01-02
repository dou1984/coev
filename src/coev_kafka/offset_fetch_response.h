// offset_fetch_response.h
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "protocol_body.h"

struct OffsetFetchResponseBlock : VEncoder, VDecoder
{
    int64_t Offset;
    int32_t LeaderEpoch;
    std::string Metadata;
    KError Err;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct OffsetFetchResponse : protocolBody
{

    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetFetchResponseBlock>>> Blocks;
    KError Err;

    OffsetFetchResponse();

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
    std::shared_ptr<OffsetFetchResponseBlock> GetBlock(const std::string &topic, int32_t partition) const;
    void AddBlock(const std::string &topic, int32_t partition, std::shared_ptr<OffsetFetchResponseBlock> block);
};
