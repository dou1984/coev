#pragma once

#include <map>
#include <string>
#include <memory>
#include <chrono>
#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "protocol_body.h"

struct alterPartitionReassignmentsErrorBlock : IEncoder, IDecoder
{
    KError errorCode;
    std::string errorMessage;

    alterPartitionReassignmentsErrorBlock() = default;
    alterPartitionReassignmentsErrorBlock(KError err, const std::string &msg) : errorCode(err), errorMessage(msg)
    {
    }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd);
};

struct AlterPartitionReassignmentsResponse : protocolBody
{
    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError ErrorCode;
    std::string ErrorMessage;
    std::map<std::string, std::map<int32_t, std::shared_ptr<alterPartitionReassignmentsErrorBlock>>> Errors;

    AlterPartitionReassignmentsResponse() = default;
    AlterPartitionReassignmentsResponse(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    void AddError(const std::string &topic, int32_t partition, KError kerror, std::string message);

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
};
