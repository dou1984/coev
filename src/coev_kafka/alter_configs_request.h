#pragma once

#include <vector>
#include <string>
#include <map>
#include <memory>

#include "config_resource_type.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct AlterConfigsResource : VDecoder, IEncoder
{
    ConfigResourceType m_type;
    std::string m_name;
    std::map<std::string, std::string> m_config_entries;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct AlterConfigsRequest : protocol_body
{
    int16_t m_version;
    std::vector<std::shared_ptr<AlterConfigsResource>> m_resources;
    bool m_validate_only = false;

    AlterConfigsRequest() = default;
    AlterConfigsRequest(int16_t v) : m_version(v)
    {
    }

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
