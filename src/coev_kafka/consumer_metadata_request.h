#pragma once

#include <string>

#include "packet_decoder.h"
#include "packet_encoder.h"
#include "coordinator_type.h"
#include "find_coordinator_request.h"
#include "api_versions.h"
#include "protocol_body.h"

struct ConsumerMetadataRequest : protocol_body
{

    int16_t m_version;
    std::string m_consumer_group;

    ConsumerMetadataRequest() : m_version(0) {}
    ConsumerMetadataRequest(int16_t v, const std::string &group)
        : m_version(v), m_consumer_group(group) {}

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
