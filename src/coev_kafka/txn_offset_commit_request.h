#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "partition_offset_metadata.h"
#include "protocol_body.h"

struct TxnOffsetCommitRequest : protocolBody
{

    int16_t Version;
    std::string TransactionalID;
    std::string GroupID;
    int64_t ProducerID;
    int16_t ProducerEpoch;
    std::map<std::string, std::vector<std::shared_ptr<PartitionOffsetMetadata>>> Topics;

    TxnOffsetCommitRequest() = default;
    TxnOffsetCommitRequest(int16_t v) : Version(v)
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
};
