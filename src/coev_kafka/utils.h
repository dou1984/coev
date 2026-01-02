#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <memory>
#include <functional>

struct Broker;

std::vector<int32_t> dupInt32Slice(const std::vector<int32_t> &input);

void withRecover(std::function<void()> fn);

struct Encoder
{
    virtual ~Encoder() = default;
    virtual int Encode(std::string &) = 0;
    virtual size_t Length() = 0;
};

struct StringEncoder : Encoder
{

    std::string data_;
    StringEncoder(const std::string &s) : data_(s)
    {
    }
    int Encode(std::string &);
    size_t Length();
};

struct ByteEncoder : Encoder
{

    std::string data_;

    ByteEncoder(const std::string &b) : data_(b)
    {
    }
    int Encode(std::string &);
    size_t Length();
};

std::function<std::chrono::milliseconds(int retries, int maxRetries)> NewExponentialBackoff(
    std::chrono::milliseconds backoff,
    std::chrono::milliseconds maxBackoff);
