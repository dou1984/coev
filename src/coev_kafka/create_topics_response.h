#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "config_source.h"
#include "protocol_body.h"

struct CreatableTopicConfigs : VEncoder, VDecoder
{
    std::string Value;
    bool ReadOnly;
    ConfigSource ConfigSource_;
    bool IsSensitive;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct CreatableTopicResult : VEncoder, VDecoder
{
    KError TopicConfigErrorCode;
    int32_t NumPartitions;
    int16_t ReplicationFactor;
    std::map<std::string, std::shared_ptr<CreatableTopicConfigs>> Configs;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct TopicError : VEncoder, VDecoder
{
    KError Err;
    std::string ErrMsg;

    std::string Error() const;
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct CreateTopicsResponse : protocolBody
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::map<std::string, std::shared_ptr<TopicError>> TopicErrors;
    std::map<std::string, std::shared_ptr<CreatableTopicResult>> TopicResults;
    CreateTopicsResponse() = default;
    CreateTopicsResponse(int16_t v) : Version(v)
    {
    }

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
