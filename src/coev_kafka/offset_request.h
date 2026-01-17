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

struct OffsetRequestBlock : versioned_decoder, versioned_encoder
{

    int32_t m_leader_epoch;
    int64_t m_timestamp;
    int32_t m_max_num_offsets;

    OffsetRequestBlock();

    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};

struct OffsetRequest : protocol_body
{

    int16_t m_version;
    IsolationLevel m_level;
    int32_t m_replica_id;
    bool m_is_replica_id_set;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetRequestBlock>>> m_blocks;

    OffsetRequest() = default;
    OffsetRequest(int16_t v) : m_version(v)
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
    void SetReplicaID(int32_t id);
    int32_t ReplicaID();
    void AddBlock(const std::string &topic, int32_t partitionID, int64_t timestamp, int32_t maxOffsets);
};

std::shared_ptr<OffsetRequest> NewOffsetRequest(const KafkaVersion &version);