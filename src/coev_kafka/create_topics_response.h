#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "config_source.h"
#include "protocol_body.h"

struct CreatableTopicConfigs : versioned_encoder, versioned_decoder
{
    std::string m_value;
    bool m_read_only;
    ConfigSource m_config_source;
    bool m_is_sensitive;

    int encode(packetEncoder &pe, int16_t version) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct CreatableTopicResult : versioned_encoder, versioned_decoder
{
    KError m_topic_config_error_code;
    int32_t m_num_partitions;
    int16_t m_replication_factor;
    std::map<std::string, std::shared_ptr<CreatableTopicConfigs>> m_configs;

    int encode(packetEncoder &pe, int16_t version) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct TopicError : versioned_encoder, versioned_decoder
{
    KError m_err;
    std::string m_err_msg;

    std::string Error() const;
    int encode(packetEncoder &pe, int16_t version) const;
    int decode(packetDecoder &pd, int16_t version);
};

struct CreateTopicsResponse : protocol_body, flexible_version
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::map<std::string, std::shared_ptr<TopicError>> m_topic_errors;
    std::map<std::string, std::shared_ptr<CreatableTopicResult>> m_topic_results;
    CreateTopicsResponse() = default;
    CreateTopicsResponse(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);
    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
