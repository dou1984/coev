#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct AlterPartitionReassignmentsBlock : IEncoder, IDecoder
{
    std::vector<int32_t> replicas;
    AlterPartitionReassignmentsBlock() = default;
    AlterPartitionReassignmentsBlock(const std::vector<int32_t> &r) : replicas(r)
    {
    }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct AlterPartitionReassignmentsRequest : protocolBody, throttleSupport
{
    std::chrono::milliseconds Timeout;
    std::map<std::string, std::map<int32_t, std::shared_ptr<AlterPartitionReassignmentsBlock>>> blocks;
    int16_t Version = 0;
    AlterPartitionReassignmentsRequest() = default;
    AlterPartitionReassignmentsRequest(int16_t v) : Version(v)
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
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;

    void AddBlock(const std::string &topic, int32_t partitionID, const std::vector<int32_t> &replicas);
};
