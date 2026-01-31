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
#include <map>

struct OffsetFetchResponseBlock : versioned_encoder, versioned_decoder
{
    int64_t m_offset;
    int32_t m_leader_epoch;
    std::string m_metadata;
    KError m_err;

    int encode(packetEncoder &pe, int16_t version) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct OffsetFetchResponse : protocol_body, flexible_version
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::unordered_map<std::string, std::map<int32_t, std::shared_ptr<OffsetFetchResponseBlock>>> m_blocks;
    KError m_err;

    OffsetFetchResponse();

    void set_version(int16_t v);
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
    std::shared_ptr<OffsetFetchResponseBlock> get_block(const std::string &topic, int32_t partition) const;
    void add_block(const std::string &topic, int32_t partition, std::shared_ptr<OffsetFetchResponseBlock> block);
};
