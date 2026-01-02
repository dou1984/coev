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

struct AlterClientQuotasEntryResponse : protocolBody
{
    int16_t Version;
    KError ErrorCode;
    std::string ErrorMsg;
    std::vector<QuotaEntityComponent> Entity;
    AlterClientQuotasEntryResponse() = default;
    AlterClientQuotasEntryResponse(int16_t v) : Version(v)
    {
    }
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    void setVersion(int16_t version);
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
};

struct AlterClientQuotasResponse : protocolBody, throttleSupport
{
    int16_t Version;
    std::chrono::milliseconds ThrottleTime;
    std::vector<AlterClientQuotasEntryResponse> Entries;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
