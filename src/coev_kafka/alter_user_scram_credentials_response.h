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
    std::string User;
    KError ErrorCode;
    std::string ErrorMessage;
};

struct AlterUserScramCredentialsResponse : protocolBody
{
    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    std::vector<std::shared_ptr<AlterUserScramCredentialsResult>> Results;

    AlterUserScramCredentialsResponse() = default;
    AlterUserScramCredentialsResponse(int16_t v) : Version(v)
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
    std::chrono::milliseconds throttleTime() const;
};
