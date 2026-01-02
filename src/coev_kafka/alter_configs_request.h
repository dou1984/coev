#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

#include "config_resource_type.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct AlterConfigsResource : VDecoder, IEncoder
{
    ConfigResourceType Type;
    std::string Name;
    std::map<std::string, std::string> ConfigEntries;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct AlterConfigsRequest : protocolBody
{
    int16_t Version;
    std::vector<std::shared_ptr<AlterConfigsResource>> Resources;
    bool ValidateOnly = false;

    AlterConfigsRequest() = default;
    AlterConfigsRequest(int16_t v) : Version(v)
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
