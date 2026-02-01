#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "version.h"
#include "api_versions.h"
#include "describe_client_quotas_response.h"
#include "errors.h"
#include "protocol_body.h"

struct AlterClientQuotasEntryResponse : protocol_body
{
    int16_t m_version;
    KError m_error_code;
    std::string m_error_msg;
    std::vector<QuotaEntityComponent> m_entity;
    AlterClientQuotasEntryResponse() = default;
    AlterClientQuotasEntryResponse(int16_t v) : m_version(v)
    {
    }
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    void set_version(int16_t version);
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};

struct AlterClientQuotasResponse : protocol_body, throttle_support
{
    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    std::vector<AlterClientQuotasEntryResponse> m_entries;

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
