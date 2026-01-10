#pragma once

#include <string>
#include <vector>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "errors.h"
#include "api_versions.h"
#include "version.h"
#include "quota_types.h"
#include "protocol_body.h"

struct QuotaFilterComponent
{
    std::string m_entity_type;
    QuotaMatchType m_match_type;
    std::string m_match;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeClientQuotasRequest : protocol_body
{
    int16_t m_version;
    std::vector<QuotaFilterComponent> m_components;
    bool m_strict;  
    DescribeClientQuotasRequest() = default;
    DescribeClientQuotasRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion required_version() const;
};
std::shared_ptr<DescribeClientQuotasRequest> NewDescribeClientQuotasRequest(KafkaVersion kafkaVersion, const std::vector<QuotaFilterComponent> &components, bool strict);
