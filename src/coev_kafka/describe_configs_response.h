#pragma once

#include <string>
#include <vector>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "config_resource_type.h"
#include "config_source.h"
#include "protocol_body.h"

struct ConfigSynonym : versioned_decoder, versioned_encoder
{
    std::string m_config_name;
    std::string m_config_value;
    ConfigSource m_source;

    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};

struct ConfigEntry : versioned_decoder, versioned_encoder
{
    std::string m_name;
    std::string m_value;
    bool m_read_only;
    bool m_default;
    bool m_sensitive;
    ConfigSource m_source;
    std::vector<std::shared_ptr<ConfigSynonym>> m_synonyms;

    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};

struct ResourceResponse : versioned_decoder, versioned_encoder
{
    int16_t m_error_code;
    std::string m_error_msg;
    ConfigResourceType m_type;
    std::string m_name;
    std::vector<std::shared_ptr<ConfigEntry>> m_configs;

    int encode(packetEncoder &pe, int16_t version);
    int decode(packetDecoder &pd, int16_t version);
};

struct DescribeConfigsResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<std::shared_ptr<ResourceResponse>> m_resources;

    void set_version(int16_t v);
    int encode(packetEncoder &pe);
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
