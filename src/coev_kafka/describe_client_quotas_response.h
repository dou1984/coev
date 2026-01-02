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
    std::string EntityType;
    QuotaMatchType MatchType;
    std::string Name;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeClientQuotasEntry
{
    std::vector<QuotaEntityComponent> Entity;
    std::map<std::string, double> Values;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct DescribeClientQuotasResponse : protocolBody
{

    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    KError ErrorCode;
    std::string ErrorMsg;
    std::vector<DescribeClientQuotasEntry> Entries;

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
    std::chrono::milliseconds throttleTime() const;
};
