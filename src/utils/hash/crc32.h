/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2025, Zhao Yun Shan
 *
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include "hash.h"

namespace coev::crc32
{
    constexpr inline size_t kSize = 4;
    enum Polynomial
    {
        IEEE = 0xedb88320,
        Castagnoli = 0x82f63b78,
        Koopman = 0xeb31d82e,

    };

    using Handle = uint32_t *;
    struct Digest
    {
        uint32_t crc;
        Handle hdl;
    };

    struct CRC32 : Hash32
    {
        CRC32(Handle handle);
        CRC32(Polynomial poly);
        void Reset();
        void Update(const char *data, size_t length);
        uint32_t Final() const;
        std::string Sum(const std::string &in) const;
        size_t Size() const { return kSize; }
        size_t BlockSize() const { return 1; }
        std::string MarshalBinary() const;
        bool UnmarshalBinary(const std::string &b);
        std::shared_ptr<Hash32> Clone() const;
        Digest GetDigest() const;

        Digest m_digest;
    };

    uint32_t Update(uint32_t crc, const char *data, size_t length, Handle tab);

    uint32_t Checksum(const char *data, size_t length, Handle tab);
    uint32_t ChecksumIEEE(const char *data, size_t length);

    std::shared_ptr<Hash32> NewIEEE();

    void BEAppendUint32(std::string &str, uint32_t value);
    uint32_t BEUint32(const std::string &str, size_t offset);

}
