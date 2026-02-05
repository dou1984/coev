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
    virtual size_t Length() = 0;
};

struct StringEncoder : Encoder
{

    std::string m_data;
    StringEncoder(const std::string &s) : m_data(s)
    {
    }
    int Encode(std::string &);
    size_t Length();
};

struct ByteEncoder : Encoder
{

    std::string m_data;

    ByteEncoder(const std::string &b) : m_data(b)
    {
    }
    int Encode(std::string &);
    size_t Length();
};

std::function<std::chrono::milliseconds(int retries, int maxRetries)> NewExponentialBackoff(
    std::chrono::milliseconds backoff,
    std::chrono::milliseconds maxBackoff);
