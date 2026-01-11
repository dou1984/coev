#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "errors.h"
#include "version.h"
#include "quota_types.h"
#include "protocol_body.h"

struct QuotaEntityComponent
{
    std::string m_entity_type;
    QuotaMatchType m_match_type;
    std::string m_name;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeClientQuotasEntry
{
    std::vector<QuotaEntityComponent> m_entity;
    std::map<std::string, double> m_values;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeClientQuotasResponse : protocol_body
{

    int16_t m_version;
    std::chrono::milliseconds m_throttle_time;
    KError m_error_code;
    std::string m_error_msg;
    std::vector<DescribeClientQuotasEntry> m_entries;

    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
