#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <cstdint>
#include "isolation_level.h"
#include "config.h"
#include "version.h"
#include "api_versions.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct OffsetRequestBlock : VDecoder, VEncoder
{

    int32_t currentLeaderEpoch;
    int64_t timestamp;
    int32_t maxNumOffsets;

    OffsetRequestBlock();

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct OffsetRequest : protocolBody
{

    int16_t Version;
    IsolationLevel Level;
    int32_t replicaID;
    bool isReplicaIDSet;
    std::unordered_map<std::string, std::unordered_map<int32_t, std::shared_ptr<OffsetRequestBlock>>> blocks;

    OffsetRequest() = default;
    OffsetRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    void SetReplicaID(int32_t id);
    int32_t ReplicaID();
    void AddBlock(const std::string &topic, int32_t partitionID, int64_t timestamp, int32_t maxOffsets);
};

std::shared_ptr<OffsetRequest> NewOffsetRequest(const KafkaVersion &version);