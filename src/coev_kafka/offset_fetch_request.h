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

struct OffsetFetchRequest : protocolBody
{
    int16_t Version;
    std::string ConsumerGroup;
    bool RequireStable;

    OffsetFetchRequest();
    OffsetFetchRequest(int16_t v) : Version(v)
    {
    }

    static std::shared_ptr<OffsetFetchRequest> NewOffsetFetchRequest(const KafkaVersion &version, const std::string &group, const std::map<std::string, std::vector<int32_t>> &partitions);

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
    void ZeroPartitions();
    void AddPartition(const std::string &topic, int32_t partitionID);

private:
    std::map<std::string, std::vector<int32_t>> partitions;
};
