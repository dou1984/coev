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

struct AlterUserScramCredentialsResponse : protocol_body
{
    int16_t m_version = 0;
    std::chrono::milliseconds m_throttle_time;
    std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> m_results;

    AlterUserScramCredentialsResponse() = default;
    AlterUserScramCredentialsResponse(int16_t v) : m_version(v)
    {
    }
    void set_version(int16_t v);

    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool is_valid_version() const;
    bool isFlexible() const;
    bool isFlexibleVersion(int16_t version) const;
    KafkaVersion required_version() const;
    std::chrono::milliseconds throttleTime() const;
};
