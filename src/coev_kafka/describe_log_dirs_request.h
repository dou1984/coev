#pragma once

#include <string>
#include <vector>

#include "packet_encoder.h"
#include "packet_decoder.h"

#include "api_versions.h"
#include "version.h"
#include "protocol_body.h"

struct DescribeLogDirsRequestTopic
{
    std::string Topic;
    std::vector<int32_t> PartitionIDs;
};

struct DescribeLogDirsRequest : protocolBody
{

    int16_t Version;
    std::vector<DescribeLogDirsRequestTopic> DescribeTopics;
    DescribeLogDirsRequest() = default;
    DescribeLogDirsRequest(int16_t v) : Version(v)
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
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
};
