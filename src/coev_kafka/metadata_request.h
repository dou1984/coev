#pragma once

#include <array>
#include <string>

#include <vector>
#include <string>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct MetadataRequest : protocolBody
{
    int16_t Version;
    std::vector<std::string> Topics;
    bool AllowAutoTopicCreation;
    bool IncludeClusterAuthorizedOperations;
    bool IncludeTopicAuthorizedOperations;

    MetadataRequest() = default;
    MetadataRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion requiredVersion() const;
};

std::shared_ptr<MetadataRequest> NewMetadataRequest(KafkaVersion version, const std::vector<std::string> &topics);