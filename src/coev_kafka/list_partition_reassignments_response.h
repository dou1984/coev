#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"
#include "api_versions.h"
#include "protocol_body.h"

struct PartitionReplicaReassignmentsStatus
{
    std::vector<int32_t> Replicas;
    std::vector<int32_t> AddingReplicas;
    std::vector<int32_t> RemovingReplicas;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct ListPartitionReassignmentsResponse : protocolBody
{

    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError ErrorCode;
    std::string ErrorMessage;
    std::map<std::string, std::map<int32_t, std::shared_ptr<PartitionReplicaReassignmentsStatus>>> TopicStatus;

    void setVersion(int16_t v);
    void AddBlock(
        const std::string &topic,
        int32_t partition,
        const std::vector<int32_t> &replicas,
        const std::vector<int32_t> &addingReplicas,
        const std::vector<int32_t> &removingReplicas);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;

    std::chrono::milliseconds throttleTime() const;
};
