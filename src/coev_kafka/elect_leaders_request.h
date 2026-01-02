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

struct ElectLeadersRequest : protocolBody
{

    int16_t Version = 0;
    ElectionType Type = ElectionType::Preferred;
    std::unordered_map<std::string, std::vector<int32_t>> TopicPartitions;
    std::chrono::milliseconds Timeout;

    ElectLeadersRequest() = default;
    ElectLeadersRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
};
