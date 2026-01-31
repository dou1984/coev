// offset_fetch_request.h
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"

#include "api_versions.h"
#include "protocol_body.h"

struct OffsetFetchRequest : protocol_body, flexible_version
{
    int16_t m_version;
    std::string m_consumer_group;
    bool m_require_stable;

    OffsetFetchRequest();
    OffsetFetchRequest(int16_t v) : m_version(v)
    {
    }
    OffsetFetchRequest(const KafkaVersion &version, const std::string &group, const std::map<std::string, std::vector<int32_t>> &partitions);

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
    void ZeroPartitions();
    void AddPartition(const std::string &topic, int32_t partitionID);

private:
    std::map<std::string, std::vector<int32_t>> m_partitions;
};
