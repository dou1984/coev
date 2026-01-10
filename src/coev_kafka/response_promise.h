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
    std::chrono::time_point<std::chrono::system_clock> m_request_time;
    int32_t m_correlation_id = 0;
    std::shared_ptr<protocol_body> m_response;
    std::function<void(std::string &, KError)> m_handler;
    coev::co_channel<std::string> m_packets;
    coev::co_channel<KError> m_errors;

    void Handle(std::string &packets_, KError err);
};
