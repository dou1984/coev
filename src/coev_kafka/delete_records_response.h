#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "protocol_body.h"

struct DeleteRecordsResponsePartition : IEncoder, VEncoder
{
    int64_t LowWatermark;
    KError Err;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DeleteRecordsResponseTopic : VDecoder, IEncoder
{
    std::map<int32_t, std::shared_ptr<DeleteRecordsResponsePartition>> Partitions;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    ~DeleteRecordsResponseTopic();
};

struct DeleteRecordsResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::map<std::string, std::shared_ptr<DeleteRecordsResponseTopic>> Topics;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;

    ~DeleteRecordsResponse();
};
