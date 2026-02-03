#include "crc32.h"
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <endian.h>

namespace coev::crc32
{

    constexpr const char *magic = "crc\x01";
    constexpr size_t marshaled_size = 4 + 4 + 4;
    constexpr size_t handle_size = 256;

    static uint32_t SimpleUpdate(uint32_t crc, const char *data, size_t length, Handle tab)
    {
        crc = ~crc;
        for (size_t i = 0; i < length; ++i)
        {
            crc = tab[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        }
        return ~crc;
    }

    Handle SimpleMakeHandle(uint32_t poly, Handle table)
    {
        for (uint32_t i = 0; i < handle_size; ++i)
        {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j)
            {
                if (crc & 1)
                {
                    crc = (crc >> 1) ^ poly;
                }
                else
                {
                    crc >>= 1;
                }
            }
            table[i] = crc;
        }
        return table;
    }

    static uint32_t ieee[handle_size];
    static uint32_t castagnoli[handle_size];
    static uint32_t koopman[handle_size];
    static uint32_t default_handle[handle_size];
    auto _ieee = SimpleMakeHandle(IEEE, ieee);
    auto _castagnoli = SimpleMakeHandle(Castagnoli, castagnoli);
    auto _koopman = SimpleMakeHandle(Koopman, koopman);

    Handle MakeHandle(Polynomial poly)
    {
        switch (poly)
        {
        case IEEE:
            return ieee;
        case Castagnoli:
            return castagnoli;
        case Koopman:
            return koopman;
        default:
            SimpleMakeHandle(poly, default_handle);
            return default_handle;
        }
    }

    uint32_t Update(uint32_t crc, const char *data, size_t length, Handle handle)
    {
        return SimpleUpdate(crc, data, length, handle);
    }

    uint32_t Checksum(const char *data, size_t length, Handle handle)
    {
        return Update(0, data, length, handle);
    }

    uint32_t ChecksumIEEE(const char *data, size_t length)
    {
        return Checksum(data, length, ieee);
    }

    CRC32::CRC32(Handle handle)
    {
        m_digest.crc = 0;
        m_digest.hdl = handle;
    }

    CRC32::CRC32(Polynomial poly)
    {
        m_digest.crc = 0;
        m_digest.hdl = MakeHandle(poly);
    }

    void CRC32::Reset()
    {
        m_digest.crc = 0;
    }

    void CRC32::Update(const char *data, size_t length)
    {
        m_digest.crc = crc32::Update(m_digest.crc, data, length, m_digest.hdl);
    }

    uint32_t CRC32::Final() const
    {
        return m_digest.crc;
    }

    std::string CRC32::Sum(const std::string &in) const
    {
        std::string result = in;
        uint32_t s = Final();
        BEAppendUint32(result, s);
        return result;
    }

    std::string CRC32::MarshalBinary() const
    {
        std::string result;
        result.reserve(marshaled_size);
        result.append(magic, 4);

        uint32_t tableSum = 0;
        if (m_digest.hdl != nullptr)
        {
            std::vector<char> tableData(reinterpret_cast<const char *>(m_digest.hdl),
                                        reinterpret_cast<const char *>(m_digest.hdl) + sizeof(Handle));
            tableSum = ChecksumIEEE(tableData.data(), tableData.size());
        }

        BEAppendUint32(result, tableSum);
        BEAppendUint32(result, m_digest.crc);

        return result;
    }

    bool CRC32::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaled_size)
        {
            return false;
        }

        uint32_t expectedTableSum = 0;
        if (m_digest.hdl != nullptr)
        {
            std::vector<char> tableData(reinterpret_cast<const char *>(m_digest.hdl),
                                        reinterpret_cast<const char *>(m_digest.hdl) + sizeof(Handle));
            expectedTableSum = ChecksumIEEE(tableData.data(), tableData.size());
        }

        uint32_t storedTableSum = BEUint32(b, 4);
        if (expectedTableSum != storedTableSum)
        {
            return false;
        }

        m_digest.crc = BEUint32(b, 8);

        return true;
    }

    std::shared_ptr<Hash32> CRC32::Clone() const
    {
        return std::make_shared<CRC32>(*this);
    }

    Digest CRC32::GetDigest() const
    {
        return m_digest;
    }

    std::shared_ptr<Hash32> NewIEEE()
    {
        return std::make_shared<CRC32>(ieee);
    }

    void BEAppendUint32(std::string &str, uint32_t value)
    {
        uint32_t be_value = htonl(value);
        str.append(reinterpret_cast<const char *>(&be_value), sizeof(be_value));
    }

    uint32_t BEUint32(const std::string &str, size_t offset)
    {
        uint32_t be_value;
        std::memcpy(&be_value, str.data() + offset, sizeof(be_value));
        return ntohl(be_value);
    }

}
