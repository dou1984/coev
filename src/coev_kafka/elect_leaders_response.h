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

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

class ElectLeadersResponse : public protocol_body
{
public:
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_error_code;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionResult>>> m_replica_election_results;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t header_version()const;
    bool is_valid_version() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
