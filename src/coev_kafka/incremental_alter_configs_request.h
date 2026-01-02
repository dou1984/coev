#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "version.h"
#include "config_resource_type.h"
#include "api_versions.h"
#include "protocol_body.h"
#include "incremental_alter_configs_operation.h"

struct IncrementalAlterConfigsEntry : IEncoder, VDecoder
{
    IncrementalAlterConfigsOperation Operation;
    std::string Value;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct IncrementalAlterConfigsResource : IEncoder, VDecoder
{
    ConfigResourceType Type;
    std::string Name;
    std::map<std::string, IncrementalAlterConfigsEntry> ConfigEntries;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct IncrementalAlterConfigsRequest : protocolBody
{

    int16_t Version = 0;
    std::vector<std::shared_ptr<IncrementalAlterConfigsResource>> Resources;
    bool ValidateOnly = false;
    IncrementalAlterConfigsRequest() = default;
    IncrementalAlterConfigsRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible();
    static bool isFlexibleVersion(int16_t ver);
    KafkaVersion requiredVersion() const;
};
