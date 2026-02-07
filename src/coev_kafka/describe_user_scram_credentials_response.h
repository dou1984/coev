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
    std::vector<UserScramCredentialsResponseInfo> m_credential_infos;
};

struct DescribeUserScramCredentialsResponse : protocol_body, flexible_version
{
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    KError m_error_code;
    std::string m_error_message;
    std::vector<DescribeUserScramCredentialsResult> m_results;

    void set_version(int16_t v);
    int encode(packet_encoder &pe) const;
    int decode(packet_decoder &pd, int16_t version);
    int16_t key() const;
    int16_t version() const;
    int16_t header_version() const;
    bool is_valid_version() const;
    bool is_flexible() const;
    bool is_flexible_version(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttle_time() const;
};
