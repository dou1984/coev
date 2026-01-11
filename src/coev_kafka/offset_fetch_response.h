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
    int64_t m_offset;
    int32_t m_leader_epoch;
    std::string m_metadata;
    KError m_err;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct OffsetFetchResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetFetchResponseBlock>>> m_blocks;
    KError m_err;

    OffsetFetchResponse();

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    static bool is_flexible_version(int16_t version);
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    std::shared_ptr<OffsetFetchResponseBlock> GetBlock(const std::string &topic, int32_t partition) const;
    void AddBlock(const std::string &topic, int32_t partition, std::shared_ptr<OffsetFetchResponseBlock> block);
};
