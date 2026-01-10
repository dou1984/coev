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
    std::string m_topic;
    std::vector<int32_t> m_partition_ids;
};

struct DescribeLogDirsRequest : protocol_body
{

    int16_t m_version;
    std::vector<DescribeLogDirsRequestTopic> m_describe_topics;
    DescribeLogDirsRequest() = default;
    DescribeLogDirsRequest(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion required_version() const;
};
