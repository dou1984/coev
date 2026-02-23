/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <cstdint>
#include <string>
#include <memory>

struct Hash32
{
    virtual ~Hash32() = default;
    virtual void Reset() = 0;
    virtual void Update(const char *data, size_t length) = 0;
    virtual uint32_t Final() const = 0;
    virtual std::string Sum(const std::string &in) const = 0;
    virtual size_t Size() const { return 4; }
    virtual size_t BlockSize() const { return 1; }
    virtual std::string MarshalBinary() const = 0;
    virtual bool UnmarshalBinary(const std::string &b) = 0;
    virtual std::shared_ptr<Hash32> Clone() const = 0;
};

struct Hash64
{
    virtual ~Hash64() = default;
    virtual void Reset() = 0;
    virtual void Update(const char *data, size_t length) = 0;
    virtual uint64_t Final() const = 0;
    virtual std::string Sum(const std::string &in) const = 0;
    virtual size_t Size() const { return 8; }
    virtual size_t BlockSize() const { return 1; }
    virtual std::string MarshalBinary() const = 0;
    virtual bool UnmarshalBinary(const std::string &b) = 0;
    virtual std::shared_ptr<Hash64> Clone() const = 0;
};

struct Hash128
{
    virtual ~Hash128() = default;
    virtual void Reset() = 0;
    virtual void Update(const char *data, size_t length) = 0;
    virtual std::array<uint64_t, 2> Final() const = 0;
    virtual std::string Sum(const std::string &in) const = 0;
    virtual size_t Size() const { return 16; }
    virtual size_t BlockSize() const { return 1; }
    virtual std::string MarshalBinary() const = 0;
    virtual bool UnmarshalBinary(const std::string &b) = 0;
    virtual std::shared_ptr<Hash128> Clone() const = 0;
};