#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct TopicDetail : IEncoder, VDecoder
{
    int32_t NumPartitions;
    int16_t ReplicationFactor;
    std::map<int32_t, std::vector<int32_t>> ReplicaAssignment;
    std::map<std::string, std::string> ConfigEntries;

    TopicDetail() = default;
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct CreateTopicsRequest : protocolBody
{

    int16_t Version;
    std::map<std::string, std::shared_ptr<TopicDetail>> TopicDetails;
    std::chrono::milliseconds Timeout;
    bool ValidateOnly;

    CreateTopicsRequest() = default;
    CreateTopicsRequest(int16_t v) : Version(v)
    {
    }
    CreateTopicsRequest(int16_t v, int64_t timeoutMs, bool validateOnly) : Version(v), Timeout(timeoutMs), ValidateOnly(validateOnly)
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
};

std::shared_ptr<CreateTopicsRequest> NewCreateTopicsRequest(const KafkaVersion &Version_, std::map<std::string, std::shared_ptr<TopicDetail>> topicDetails, int64_t timeoutMs, bool validateOnly);
