#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include "hash.h"

namespace coev::fnv
{

    constexpr uint32_t offset32 = 2166136261;
    constexpr uint64_t offset64 = 14695981039346656037ULL;
    constexpr uint64_t offset128Lower = 0x62b821756295c58dULL;
    constexpr uint64_t offset128Higher = 0x6c62272e07bb0142ULL;
    constexpr uint32_t prime32 = 16777619;
    constexpr uint64_t prime64 = 1099511628211ULL;
    constexpr uint64_t prime128Lower = 0x13bULL;
    constexpr uint32_t prime128Shift = 24;

    struct FNV32 : Hash32
    {
        FNV32();
        void Reset();
        void Update(const char *data, size_t length);
        uint32_t Final() const;
        std::string Sum(const std::string &in) const;
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash32> Clone() const;

        uint32_t hash_;
    };

    struct FNV32a : Hash32
    {
        FNV32a();
        void Reset();
        void Update(const char *data, size_t length);
        uint32_t Final() const;
        std::string Sum(const std::string &in) const;
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash32> Clone() const;

        uint32_t hash_;
    };

    struct FNV64 : Hash64
    {
        FNV64();
        void Reset();
        void Update(const char *data, size_t length);
        uint64_t Final() const;
        std::string Sum(const std::string &in) const;
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash64> Clone() const;

        uint64_t hash_;
    };

    struct FNV64a : Hash64
    {
        FNV64a();
        void Reset();
        void Update(const char *data, size_t length);
        uint64_t Final() const;
        std::string Sum(const std::string &in) const;
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash64> Clone() const;

        uint64_t hash_;
    };

    struct FNV128 : Hash128
    {
        FNV128();
        void Reset();
        void Update(const char *data, size_t length);
        std::array<uint64_t, 2> Final() const;
        std::string Sum(const std::string &in) const;
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash128> Clone() const;

        uint64_t hash_[2];
    };

    struct FNV128a : Hash128
    {
        FNV128a();
        void Reset();
        void Update(const char *data, size_t length);
        std::array<uint64_t, 2> Final() const;
        std::string Sum(const std::string &in) const;
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash128> Clone() const;

        uint64_t hash_[2];
    };

    std::shared_ptr<Hash32> New32();
    std::shared_ptr<Hash32> New32a();
    std::shared_ptr<Hash64> New64();
    std::shared_ptr<Hash64> New64a();
    std::shared_ptr<Hash128> New128();
    std::shared_ptr<Hash128> New128a();

    void BEAppendUint32(std::string &str, uint32_t value);
    void BEAppendUint64(std::string &str, uint64_t value);
    uint32_t BEUint32(const std::string &str, size_t offset);
    uint64_t BEUint64(const std::string &str, size_t offset);

}
