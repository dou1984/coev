#pragma once

#include <string>
#include <cstdint>
#include "version.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"
#include "config.h"

struct ApiVersionsRequest : protocol_body, flexible_version
{
    int16_t m_version = 0;
    mutable std::string m_client_software_name = defaultClientSoftwareName;
    mutable std::string m_client_software_version = defaultClientSoftwareVersion;
    ApiVersionsRequest() = default;
    ApiVersionsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
};
