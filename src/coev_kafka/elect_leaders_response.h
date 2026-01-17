#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "protocol_body.h"

struct PartitionResult
{
    KError m_error_code;
    std::string m_error_message;

    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};

struct ElectLeadersResponse :  protocol_body , flexible_version
{

    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_error_code;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionResult>>> m_replica_election_results;

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version() const;
    bool is_flexible() const;
     bool is_flexible_version(int16_t version)const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
