#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "config_resource_type.h"
#include "errors.h"
#include "protocol_body.h"

struct AlterConfigError
{
    KError m_err;
    std::string m_err_msg;
    std::string Error() const;
};

struct AlterConfigsResourceResponse : VDecoder, IEncoder
{
    int16_t m_error_code = 0;
    std::string m_error_msg;
    ConfigResourceType m_type;
    std::string m_name;

    int encode(class packet_encoder &pe) const;
    int decode(class packet_decoder &pd, int16_t version);
};

struct AlterConfigsResponse : protocol_body
{
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    std::vector<AlterConfigsResourceResponse> m_resources;

    AlterConfigsResponse() = default;
    AlterConfigsResponse(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
