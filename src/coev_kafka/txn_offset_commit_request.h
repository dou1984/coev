#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "partition_offset_metadata.h"
#include "protocol_body.h"

struct TxnOffsetCommitRequest : protocol_body
{

    int16_t m_version;
    std::string m_transactional_id;
    std::string m_group_id;
    int64_t m_producer_id;
    int16_t m_producer_epoch;
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> m_topics;

    TxnOffsetCommitRequest() = default;
    TxnOffsetCommitRequest(int16_t v) : m_version(v)
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
};
