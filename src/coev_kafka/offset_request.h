#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "isolation_level.h"
#include "config.h"
#include "version.h"
#include "api_versions.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct OffsetRequestBlock : VDecoder, VEncoder
{

    int32_t m_leader_epoch;
    int64_t m_timestamp;
    int32_t m_max_num_offsets;

    OffsetRequestBlock();

    int encode(packet_encoder &pe, int16_t version) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct OffsetRequest : protocol_body
{

    int16_t m_version;
    IsolationLevel m_level;
    int32_t m_replica_id;
    bool m_is_replica_id_set;
    std::unordered_map<std::string, std::map<int32_t, OffsetRequestBlock>> m_blocks;

    OffsetRequest();
    OffsetRequest(int16_t v) : m_version(v), m_level(ReadUncommitted), m_replica_id(-1), m_is_replica_id_set(false)
    {
    }
    OffsetRequest(const KafkaVersion &version);
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    void set_replica_id(int32_t id);
    int32_t replica_id();
    void add_block(const std::string &topic, int32_t partition_id, int64_t timestamp, int32_t max_offsets);
};
