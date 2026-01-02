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
    std::string EntityType;
    QuotaMatchType MatchType;
    std::string Match;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeClientQuotasRequest : protocolBody
{
    int16_t Version;
    std::vector<QuotaFilterComponent> Components;
    bool Strict;
    DescribeClientQuotasRequest() = default;
    DescribeClientQuotasRequest(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    bool isFlexible() const;
    static bool isFlexibleVersion(int16_t version);
    KafkaVersion requiredVersion() const;
};
std::shared_ptr<DescribeClientQuotasRequest> NewDescribeClientQuotasRequest(KafkaVersion kafkaVersion, const std::vector<QuotaFilterComponent> &components, bool strict);
