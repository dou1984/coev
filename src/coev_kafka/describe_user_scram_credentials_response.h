#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

#include "packet_encoder.h"
#include "packet_decoder.h"
#include "api_versions.h"
#include "version.h"
#include "errors.h"
#include "scram_formatter.h"
#include "protocol_body.h"

struct UserScramCredentialsResponseInfo
{
    ScramMechanismType Mechanism;
    int32_t Iterations;
};

struct DescribeUserScramCredentialsResult
{
    std::string User;
    KError ErrorCode;
    std::string ErrorMessage;
    std::vector<std::shared_ptr<UserScramCredentialsResponseInfo>> CredentialInfos;
};

struct DescribeUserScramCredentialsResponse : protocolBody
{
    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    KError ErrorCode;
    std::string ErrorMessage;
    std::vector<std::shared_ptr<DescribeUserScramCredentialsResult>> Results;

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
