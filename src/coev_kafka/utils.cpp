#include "utils.h"
#include "errors.h"
#include <coev/log.h>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <regex>
#include <cctype>
#include <limits>
#include <coev/coev.h>

std::vector<int32_t> dupInt32Slice(const std::vector<int32_t> &input)
{
    return input;
}


int StringEncoder::Encode(std::string &out)
{
    out.assign(data_.begin(), data_.end());
    return 0;
}

size_t StringEncoder::Length()
{
    return data_.size();
}

// ByteEncoder
int ByteEncoder::Encode(std::string &out)
{
    out = data_;
    return 0;
}

size_t ByteEncoder::Length()
{
    return data_.size();
}

std::function<std::chrono::milliseconds(int, int)> NewExponentialBackoff(
    std::chrono::milliseconds backoff,
    std::chrono::milliseconds maxBackoff)
{

    static auto defaultRetryBackoff = std::chrono::milliseconds(100);
    static auto defaultRetryMaxBackoff = std::chrono::milliseconds(1000);

    if (backoff.count() <= 0)
    {
        backoff = defaultRetryBackoff;
    }
    if (maxBackoff.count() <= 0)
    {
        maxBackoff = defaultRetryMaxBackoff;
    }

    if (backoff > maxBackoff)
    {
        LOG_CORE("Warning: backoff is greater than maxBackoff, using maxBackoff instead.");
        backoff = maxBackoff;
    }

    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_real_distribution<> dis(0.8, 1.2);

    return [backoff, maxBackoff](int retries, int /*maxRetries*/) -> std::chrono::milliseconds
    {
        if (retries <= 0)
        {
            return backoff;
        }

        double multiplier = static_cast<double>(1ULL << (retries - 1));
        auto calculated = static_cast<int64_t>(static_cast<double>(backoff.count()) * multiplier * dis(gen));
        auto bounded = std::min<int64_t>(calculated, maxBackoff.count());
        return std::chrono::milliseconds(bounded);
    };
}