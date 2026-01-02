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
    std::string Name;
    ScramMechanismType Mechanism;
};

struct AlterUserScramCredentialsUpsert
{
    std::string Name;
    ScramMechanismType Mechanism;
    int32_t Iterations = 0;
    std::string Salt;
    std::string saltedPassword;
    std::string Password;
};

struct AlterUserScramCredentialsRequest : protocolBody
{
    int16_t Version = 0;
    std::vector<AlterUserScramCredentialsDelete> Deletions;
    std::vector<AlterUserScramCredentialsUpsert> Upsertions;

    AlterUserScramCredentialsRequest() = default;
    AlterUserScramCredentialsRequest(int16_t v) : Version(v)
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
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion requiredVersion() const;
};
