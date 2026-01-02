#pragma once

#include <vector>
#include <string>
#include <memory>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "describe_client_quotas_response.h"
#include "protocol_body.h"

struct ClientQuotasOp;

struct AlterClientQuotasEntry : IEncoder, VDecoder
{
    std::vector<QuotaEntityComponent> Entity;
    std::vector<ClientQuotasOp> Ops;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct ClientQuotasOp : IEncoder, VDecoder
{
    std::string Key;
    double Value;
    bool Remove;

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);
};

struct AlterClientQuotasRequest : protocolBody
{
    AlterClientQuotasRequest() = default;
    AlterClientQuotasRequest(int16_t v) : Version(v)
    {
    }
    int16_t Version = 0;
    std::vector<AlterClientQuotasEntry> Entries;
    bool ValidateOnly = false;

    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
};
