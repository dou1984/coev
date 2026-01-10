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

struct ConfigSynonym : VDecoder, VEncoder
{
    std::string m_config_name;
    std::string m_config_value;
    ConfigSource m_source;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct ConfigEntry : VDecoder, VEncoder
{
    std::string m_name;
    std::string m_value;
    bool m_read_only;
    bool m_default;
    bool m_sensitive;
    ConfigSource m_source;
    std::vector<std::shared_ptr<ConfigSynonym>> m_synonyms;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct ResourceResponse : VDecoder, VEncoder
{
    int16_t m_error_code;
    std::string m_error_msg;
    ConfigResourceType m_type;
    std::string m_name;
    std::vector<std::shared_ptr<ConfigEntry>> m_configs;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeConfigsResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<std::shared_ptr<ResourceResponse>> m_resources;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;
};
