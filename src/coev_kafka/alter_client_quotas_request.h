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
    std::vector<QuotaEntityComponent> m_entity;
    std::vector<ClientQuotasOp> m_ops;

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct ClientQuotasOp : IEncoder, VDecoder
{
    std::string m_key;
    double m_value;
    bool m_remove = false;

    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
};

struct AlterClientQuotasRequest : protocol_body
{
    AlterClientQuotasRequest() = default;
    AlterClientQuotasRequest(int16_t v) : m_version(v)
    {
    }
    int16_t m_version = 0;
    std::vector<AlterClientQuotasEntry> m_entries;
    bool m_validate_only = false;

    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    KafkaVersion required_version() const;
};
