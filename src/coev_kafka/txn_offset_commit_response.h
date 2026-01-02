#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "utils.h"
#include "version.h"
#include "protocol_body.h"
#include "partition_error.h"

struct TxnOffsetCommitResponse : protocolBody
{

    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::map<std::string, std::vector<std::shared_ptr<PartitionError>>> Topics;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
