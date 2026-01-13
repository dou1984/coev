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

struct DescribeConfigsRequest : protocol_body
{
    int16_t m_version;
    std::vector<std::shared_ptr<ConfigResource>> m_resources;
    bool m_include_synonyms;

    DescribeConfigsRequest() = default;
    DescribeConfigsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
