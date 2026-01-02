// offset_commit_response.h
#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <chrono>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "version.h"

#include "api_versions.h"
#include "protocol_body.h"

struct OffsetCommitResponse :  protocolBody
{

    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::unordered_map<std::string, std::unordered_map<int32_t, KError>> Errors;

    OffsetCommitResponse();

    void setVersion(int16_t v);
    void AddError(const std::string &topic, int32_t partition, KError kerror);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
