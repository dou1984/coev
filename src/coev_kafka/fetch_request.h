#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "config.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "fetch_request_block.h"

struct FetchRequest : protocol_body
{
    int16_t m_version;
    int32_t m_max_wait_time;
    int32_t m_min_bytes;
    int32_t m_max_bytes;
    IsolationLevel m_isolation;
    int32_t m_session_id;
    int32_t m_session_epoch;
    std::map<std::string, std::map<int32_t, FetchRequestBlock>> m_blocks;
    std::map<std::string, std::vector<int32_t>> m_forgotten;
    std::string m_rack_id;

    FetchRequest();
    FetchRequest(int16_t v);
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    void add_block(const std::string &topic, int32_t partitionID, int64_t fetchOffset, int32_t maxBytes, int32_t leaderEpoch);
};
