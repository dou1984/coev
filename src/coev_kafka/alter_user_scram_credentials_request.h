#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "scram_formatter.h"

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct AlterUserScramCredentialsDelete
{
    std::string m_name;
    ScramMechanismType m_mechanism;
};

struct AlterUserScramCredentialsUpsert
{
    std::string m_name;
    ScramMechanismType m_mechanism;
    int32_t m_iterations = 0;
    std::string m_salt;
    std::string m_salted_password;
    std::string m_password;
};

struct AlterUserScramCredentialsRequest : protocol_body, flexible_version
{
    int16_t m_version = 0;
    std::vector<AlterUserScramCredentialsDelete> m_deletions;
    std::vector<AlterUserScramCredentialsUpsert> m_upsertions;

    AlterUserScramCredentialsRequest() = default;
    AlterUserScramCredentialsRequest(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);

    int encode(packetEncoder &pe) const;
    int decode(packetDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
};
