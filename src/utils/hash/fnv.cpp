#include "fnv.h"
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <endian.h>

namespace coev::fnv
{

    constexpr const char *magic32 = "fnv\x01";
    constexpr const char *magic32a = "fnv\x02";
    constexpr const char *magic64 = "fnv\x03";
    constexpr const char *magic64a = "fnv\x04";
    constexpr const char *magic128 = "fnv\x05";
    constexpr const char *magic128a = "fnv\x06";
    constexpr size_t marshaledSize32 = 4 + 4;
    constexpr size_t marshaledSize64 = 4 + 8;
    constexpr size_t marshaledSize128 = 4 + 16;

    void BEAppendUint32(std::string &str, uint32_t value)
    {
        uint32_t be_value = htonl(value);
        str.append(reinterpret_cast<const char *>(&be_value), sizeof(be_value));
    }

    void BEAppendUint64(std::string &str, uint64_t value)
    {
        uint64_t be_value = htobe64(value);
        str.append(reinterpret_cast<const char *>(&be_value), sizeof(be_value));
    }

    uint32_t BEUint32(const std::string &str, size_t offset)
    {
        uint32_t be_value;
        std::memcpy(&be_value, str.data() + offset, sizeof(be_value));
        return ntohl(be_value);
    }

    uint64_t BEUint64(const std::string &str, size_t offset)
    {
        uint64_t be_value;
        std::memcpy(&be_value, str.data() + offset, sizeof(be_value));
        return be64toh(be_value);
    }

    FNV32::FNV32()
    {
        Reset();
    }

    void FNV32::Reset()
    {
        m_hash = offset32;
    }

    void FNV32::Update(const char *data, size_t length)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i)
        {
            m_hash *= prime32;
            m_hash ^= bytes[i];
        }
    }

    uint32_t FNV32::Final() const
    {
        return m_hash;
    }

    std::string FNV32::Sum(const std::string &in) const
    {
        std::string result = in;
        BEAppendUint32(result, m_hash);
        return result;
    }

    std::string FNV32::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaledSize32);
        result.append(magic32, 4);
        BEAppendUint32(result, m_hash);
        return result;
    }

    bool FNV32::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic32, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize32)
        {
            return false;
        }
        m_hash = BEUint32(b, 4);
        return true;
    }
    std::shared_ptr<Hash32> FNV32::Clone() const
    {
        return std::make_shared<FNV32>(*this);
    }
    FNV32a::FNV32a()
    {
        Reset();
    }

    void FNV32a::Reset()
    {
        m_hash = offset32;
    }

    void FNV32a::Update(const char *data, size_t length)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i)
        {
            m_hash ^= bytes[i];
            m_hash *= prime32;
        }
    }

    uint32_t FNV32a::Final() const
    {
        return m_hash;
    }

    std::string FNV32a::Sum(const std::string &in) const
    {
        std::string result = in;
        BEAppendUint32(result, m_hash);
        return result;
    }

    std::string FNV32a::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaledSize32);
        result.append(magic32a, 4);
        BEAppendUint32(result, m_hash);
        return result;
    }

    bool FNV32a::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic32a, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize32)
        {
            return false;
        }
        m_hash = BEUint32(b, 4);
        return true;
    }

    std::shared_ptr<Hash32> FNV32a::Clone() const
    {
        return std::make_shared<FNV32a>(*this);
    }
    FNV64::FNV64()
    {
        Reset();
    }

    void FNV64::Reset()
    {
        m_hash = offset64;
    }

    void FNV64::Update(const char *data, size_t length)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i)
        {
            m_hash *= prime64;
            m_hash ^= bytes[i];
        }
    }

    uint64_t FNV64::Final() const
    {
        return m_hash;
    }

    std::string FNV64::Sum(const std::string &in) const
    {
        std::string result = in;
        BEAppendUint64(result, m_hash);
        return result;
    }

    std::string FNV64::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaledSize64);
        result.append(magic64, 4);
        BEAppendUint64(result, m_hash);
        return result;
    }

    bool FNV64::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic64, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize64)
        {
            return false;
        }
        m_hash = BEUint64(b, 4);
        return true;
    }

    std::shared_ptr<Hash64> FNV64::Clone() const
    {
        return std::make_shared<FNV64>(*this);
    }

    FNV64a::FNV64a()
    {
        Reset();
    }

    void FNV64a::Reset()
    {
        m_hash = offset64;
    }

    void FNV64a::Update(const char *data, size_t length)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i)
        {
            m_hash ^= bytes[i];
            m_hash *= prime64;
        }
    }

    uint64_t FNV64a::Final() const
    {
        return m_hash;
    }

    std::string FNV64a::Sum(const std::string &in) const
    {
        std::string result = in;
        BEAppendUint64(result, m_hash);
        return result;
    }

    std::string FNV64a::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaledSize64);
        result.append(magic64a, 4);
        BEAppendUint64(result, m_hash);
        return result;
    }

    bool FNV64a::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic64a, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize64)
        {
            return false;
        }
        m_hash = BEUint64(b, 4);
        return true;
    }

    std::shared_ptr<Hash64> FNV64a::Clone() const
    {
        return std::make_shared<FNV64a>(*this);
    }
    void multiply128(uint64_t &higher, uint64_t &lower, uint64_t primeLower, uint32_t primeShift)
    {
        uint64_t s0, s1;

        s0 = (lower * primeLower);
        s1 = (lower >> 32) * primeLower;
        s1 += (s0 >> 32);

        uint64_t shift_part = lower << primeShift;
        s0 += shift_part;
        s1 += (shift_part >> 32);

        s0 += higher * primeLower;
        s1 += (higher * primeLower) >> 32;

        lower = s0;
        higher = s1;
    }

    FNV128::FNV128()
    {
        Reset();
    }

    void FNV128::Reset()
    {
        m_hash[0] = offset128Higher;
        m_hash[1] = offset128Lower;
    }

    void FNV128::Update(const char *data, size_t length)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i)
        {
            multiply128(m_hash[0], m_hash[1], prime128Lower, prime128Shift);
            m_hash[1] ^= bytes[i];
        }
    }

    std::array<uint64_t, 2> FNV128::Final() const
    {
        return {m_hash[0], m_hash[1]};
    }

    std::string FNV128::Sum(const std::string &in) const
    {
        std::string result = in;
        BEAppendUint64(result, m_hash[0]);
        BEAppendUint64(result, m_hash[1]);
        return result;
    }

    std::string FNV128::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaledSize128);
        result.append(magic128, 4);
        BEAppendUint64(result, m_hash[0]);
        BEAppendUint64(result, m_hash[1]);
        return result;
    }

    bool FNV128::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic128, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize128)
        {
            return false;
        }
        m_hash[0] = BEUint64(b, 4);
        m_hash[1] = BEUint64(b, 12);
        return true;
    }

    std::shared_ptr<Hash128> FNV128::Clone() const
    {
        return std::make_shared<FNV128>(*this);
    }

    FNV128a::FNV128a()
    {
        Reset();
    }

    void FNV128a::Reset()
    {
        m_hash[0] = offset128Higher;
        m_hash[1] = offset128Lower;
    }

    void FNV128a::Update(const char *data, size_t length)
    {
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(data);
        for (size_t i = 0; i < length; ++i)
        {
            m_hash[1] ^= bytes[i];
            multiply128(m_hash[0], m_hash[1], prime128Lower, prime128Shift);
        }
    }

    std::array<uint64_t, 2> FNV128a::Final() const
    {
        return {m_hash[0], m_hash[1]};
    }

    std::string FNV128a::Sum(const std::string &in) const
    {
        std::string result = in;
        BEAppendUint64(result, m_hash[0]);
        BEAppendUint64(result, m_hash[1]);
        return result;
    }

    std::string FNV128a::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaledSize128);
        result.append(magic128a, 4);
        BEAppendUint64(result, m_hash[0]);
        BEAppendUint64(result, m_hash[1]);
        return result;
    }

    bool FNV128a::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic128a, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize128)
        {
            return false;
        }
        m_hash[0] = BEUint64(b, 4);
        m_hash[1] = BEUint64(b, 12);
        return true;
    }

    std::shared_ptr<Hash128> FNV128a::Clone() const
    {
        return std::make_shared<FNV128a>(*this);
    }

    std::shared_ptr<Hash32> New32()
    {
        return std::make_shared<FNV32>();
    }

    std::shared_ptr<Hash32> New32a()
    {
        return std::make_shared<FNV32a>();
    }

    std::shared_ptr<Hash64> New64()
    {
        return std::make_shared<FNV64>();
    }

    std::shared_ptr<Hash64> New64a()
    {
        return std::make_shared<FNV64a>();
    }

    std::shared_ptr<Hash128> New128()
    {
        return std::make_shared<FNV128>();
    }

    std::shared_ptr<Hash128> New128a()
    {
        return std::make_shared<FNV128a>();
    }

}
