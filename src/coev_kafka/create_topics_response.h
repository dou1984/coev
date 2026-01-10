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

struct CreatableTopicConfigs : VEncoder, VDecoder
{
    std::string m_value;
    bool m_read_only;
    ConfigSource m_config_source;
    bool m_is_sensitive;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct CreatableTopicResult : VEncoder, VDecoder
{
    KError m_topic_config_error_code;
    int32_t m_num_partitions;
    int16_t m_replication_factor;
    std::map<std::string, std::shared_ptr<CreatableTopicConfigs>> m_configs;

    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct TopicError : VEncoder, VDecoder
{
    KError m_err;
    std::string m_err_msg;

    std::string Error() const;
    int encode(PEncoder &pe, int16_t version);
    int decode(PDecoder &pd, int16_t version);
};

struct CreateTopicsResponse : protocol_body
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
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;
};
