#pragma once

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "config_resource_type.h"
#include "errors.h"
#include "protocol_body.h"

struct AlterConfigError
{
    KError Err;
    std::string ErrMsg;
    std::string Error() const;
};

struct AlterConfigsResourceResponse : VDecoder, IEncoder
{
    int16_t ErrorCode = 0;
    std::string ErrorMsg;
    ConfigResourceType Type;
    std::string Name;

    int encode(class PEncoder &pe);
    int decode(class PDecoder &pd, int16_t version);
};

struct AlterConfigsResponse : protocolBody
{
    int16_t Version = 0;
    std::chrono::milliseconds ThrottleTime;
    std::vector<std::shared_ptr<AlterConfigsResourceResponse>> Resources;

    AlterConfigsResponse() = default;
    AlterConfigsResponse(int16_t v) : Version(v)
    {
    }
    void setVersion(int16_t v);
    int encode(PEncoder &pe);
    int decode(PDecoder &pd, int16_t version);

    int16_t key() const;
    int16_t version() const;
    int16_t headerVersion() const;
    bool isValidVersion() const;
    KafkaVersion requiredVersion() const;
    std::chrono::milliseconds throttleTime() const;
};
