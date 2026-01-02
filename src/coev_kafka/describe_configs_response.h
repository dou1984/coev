#pragma once

#include <string>
#include <vector>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "config_resource_type.h"
#include "config_source.h"
#include "protocol_body.h"

struct ConfigSynonym : VDecoder, VEncoder
{
    std::string ConfigName;
    std::string ConfigValue;
    ConfigSource Source;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct ConfigEntry : VDecoder, VEncoder
{
    std::string Name;
    std::string Value;
    bool ReadOnly;
    bool Default;
    bool Sensitive;
    ConfigSource Source;
    std::vector<std::shared_ptr<ConfigSynonym>> Synonyms;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct ResourceResponse : VDecoder, VEncoder
{
    int16_t ErrorCode;
    std::string ErrorMsg;
    ConfigResourceType Type;
    std::string Name;
    std::vector<std::shared_ptr<ConfigEntry>> Configs;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeConfigsResponse : protocolBody
{

    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::vector<std::shared_ptr<ResourceResponse>> Resources;

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
