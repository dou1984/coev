#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <memory>
#include <functional>

struct Encoder
{
    virtual ~Encoder() = default;
    virtual int Encode(std::string &) = 0;
    virtual size_t Length() const = 0;
    virtual operator bool() const = 0;
};

struct StringEncoder : Encoder
{

    std::string m_data;
    StringEncoder() = default;
    StringEncoder(const std::string &s) : m_data(s)
    {
    }
    bool Empty() const;
    int Encode(std::string &);
    size_t Length() const;
    operator bool() const;
};

struct ByteEncoder : Encoder
{

    std::string m_data;
    ByteEncoder() = default;
    ByteEncoder(const std::string &b) : m_data(b)
    {
    }
    bool Empty() const;
    int Encode(std::string &);
    size_t Length() const;
    operator bool() const;
};

std::function<std::chrono::milliseconds(int retries, int maxRetries)> NewExponentialBackoff(
    std::chrono::milliseconds backoff,
    std::chrono::milliseconds maxBackoff);
