#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "election_type.h"
#include "protocol_body.h"

struct ElectLeadersRequest : protocol_body
{

    int16_t m_version = 0;
    ElectionType m_type = ElectionType::Preferred;
    std::unordered_map<std::string, std::vector<int32_t>> m_topic_partitions;
    std::chrono::milliseconds m_timeout;

    ElectLeadersRequest() = default;
    ElectLeadersRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion required_version() const;
};
