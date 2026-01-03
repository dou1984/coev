#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <coev/coev.h>
#include "protocol_body.h"
#include "errors.h"

struct ResponsePromise
{
    std::chrono::time_point<std::chrono::system_clock> RequestTime;
    int32_t CorrelationID;
    std::shared_ptr<protocolBody> Response;
    std::function<void(std::string &, KError)> Handler;
    coev::co_channel<std::string> Packets;
    coev::co_channel<KError> Errors;

    void Handle(std::string &packets_, KError err);
};
