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
    KError ErrorCode;
    std::string ErrorMessage;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

class ElectLeadersResponse : public protocolBody
{
public:
    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError ErrorCode;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<PartitionResult>>> ReplicaElectionResults;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version()const;
    int16_t headerVersion()const;
    bool isValidVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
