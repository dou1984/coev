#pragma once

#include <string>
#include <vector>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"
#include "config_resource_type.h"

struct DescribeConfigsRequest : protocolBody
{
    int16_t Version;
    std::vector<std::shared_ptr<ConfigResource>> Resources;
    bool IncludeSynonyms;

    DescribeConfigsRequest() = default;
    DescribeConfigsRequest(int16_t v) : Version(v)
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
