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

struct MetadataRequest : protocol_body , flexible_version
{
    int16_t m_version;
    std::vector<std::string> m_topics;
    bool m_allow_auto_topic_creation;
    bool m_include_cluster_authorized_operations;
    bool m_include_topic_authorized_operations;

    MetadataRequest() = default;
    MetadataRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);

    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
};

std::shared_ptr<MetadataRequest> NewMetadataRequest(KafkaVersion version, const std::vector<std::string> &topics);