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

struct IncrementalAlterConfigsEntry : IEncoder, versionedDecoder
{
    IncrementalAlterConfigsOperation m_operation;
    std::string m_value;

    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
};

struct IncrementalAlterConfigsResource : IEncoder, versionedDecoder
{
    ConfigResourceType m_type;
    std::string m_name;
    std::map<std::string, IncrementalAlterConfigsEntry> m_config_entries;

    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
};

struct IncrementalAlterConfigsRequest : protocol_body
{

    int16_t m_version = 0;
    std::vector<std::shared_ptr<IncrementalAlterConfigsResource>> m_resources;
    bool m_validate_only = false;
    IncrementalAlterConfigsRequest() = default;
    IncrementalAlterConfigsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible();
    static bool is_flexible_version(int16_t ver);
    KafkaVersion required_version() const;
};
