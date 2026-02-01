#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "version.h"
#include "errors.h"
#include "packet_encoder.h"
#include "packet_decoder.h"
#include "protocol_body.h"

struct AlterUserScramCredentialsResult
{
    std::string m_user;
    KError m_error_code = ErrNoError;
    std::string m_error_message;
};

struct AlterUserScramCredentialsResponse : protocol_body, flexible_version
{
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> m_results;

    AlterUserScramCredentialsResponse() = default;
    AlterUserScramCredentialsResponse(int16_t v) : m_version(v)
    {
    }
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
