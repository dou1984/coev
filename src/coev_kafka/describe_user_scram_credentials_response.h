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
    ScramMechanismType m_mechanism;
    int32_t m_iterations;
};

struct DescribeUserScramCredentialsResult
{
    std::string m_user;
    KError m_error_code;
    std::string m_error_message;
    std::vector<std::shared_ptr<UserScramCredentialsResponseInfo>> m_credential_infos;
};

struct DescribeUserScramCredentialsResponse : protocol_body
{
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_error_code;
    std::string m_error_message;
    std::vector<std::shared_ptr<DescribeUserScramCredentialsResult>> m_results;

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
    std::chrono::milliseconds throttleTime() const;
};
