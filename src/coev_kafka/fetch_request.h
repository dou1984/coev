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

struct FetchRequest : protocolBody
{
    int16_t Version;
    int32_t MaxWaitTime;
    int32_t MinBytes;
    int32_t MaxBytes;
    IsolationLevel Isolation;
    int32_t SessionID;
    int32_t SessionEpoch;
    std::map<std::string, std::map<int32_t, std::shared_ptr<FetchRequestBlock>>> blocks;
    std::map<std::string, std::vector<int32_t>> forgotten;
    std::string RackID;

    FetchRequest();
    FetchRequest(int16_t v);
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    void AddBlock(const std::string &topic, int32_t partitionID, int64_t fetchOffset, int32_t maxBytes, int32_t leaderEpoch);
};
