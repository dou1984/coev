#include "crc32.h"
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <endian.h>

namespace coev::crc32
{

    constexpr const char *magic = "crc\x01";
    constexpr size_t marshaledSize = 4 + 4 + 4;

    static uint32_t SimpleUpdate(uint32_t crc, const Table *tab, const char *data, size_t length)
    {
        crc = ~crc;
        for (size_t i = 0; i < length; ++i)
        {
            crc = tab->data[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
        }
        return ~crc;
    }

    TablePtr SimpleMakeTable(uint32_t poly)
    {
        TablePtr table = std::make_shared<Table>();
        for (uint32_t i = 0; i < 256; ++i)
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
            table->data[i] = crc;
        }
        return table;
    }

    static TablePtr ieeeTable = SimpleMakeTable(IEEE);
    static TablePtr castagnoliTable = nullptr;
    static TablePtr koopmanTable = nullptr;

    static bool castagnoliInitialized = false;
    static bool koopmanInitialized = false;

    TablePtr MakeTable(Polynomial poly)
    {
        switch (poly)
        {
        case IEEE:
            return ieeeTable;
        case Castagnoli:
            if (!castagnoliInitialized)
            {
                castagnoliTable = SimpleMakeTable(Castagnoli);
                castagnoliInitialized = true;
            }
            return castagnoliTable;
        case Koopman:
            if (!koopmanInitialized)
            {
                koopmanTable = SimpleMakeTable(Koopman);
                koopmanInitialized = true;
            }
            return koopmanTable;
        default:
            return SimpleMakeTable(static_cast<uint32_t>(poly));
        }
    }

    uint32_t Update(uint32_t crc, const TablePtr &tab, const char *data, size_t length)
    {
        return SimpleUpdate(crc, tab.get(), data, length);
    }

    uint32_t Checksum(const char *data, size_t length, const TablePtr &tab)
    {
        return Update(0, tab, data, length);
    }

    uint32_t ChecksumIEEE(const char *data, size_t length)
    {
        return Checksum(data, length, ieeeTable);
    }

    CRC32::CRC32(const TablePtr &table)
    {
        digest_.crc = 0;
        digest_.tab = table;
    }

    CRC32::CRC32(Polynomial poly)
    {
        digest_.crc = 0;
        digest_.tab = MakeTable(poly);
    }

    void CRC32::Reset()
    {
        digest_.crc = 0;
    }

    void CRC32::Update(const char *data, size_t length)
    {
        digest_.crc = crc32::Update(digest_.crc, digest_.tab, data, length);
    }

    uint32_t CRC32::Final() const
    {
        return digest_.crc;
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
        result.reserve(marshaledSize);
        result.append(magic, 4);

        uint32_t tableSum = 0;
        if (digest_.tab != nullptr)
        {
            std::vector<char> tableData(reinterpret_cast<const char *>(digest_.tab.get()),
                                        reinterpret_cast<const char *>(digest_.tab.get()) + sizeof(Table));
            tableSum = ChecksumIEEE(tableData.data(), tableData.size());
        }

        BEAppendUint32(result, tableSum);
        BEAppendUint32(result, digest_.crc);

        return result;
    }

    bool CRC32::UnmarshalBinary(const std::string &b)
    {
        if (b.size() < 4 || std::memcmp(b.data(), magic, 4) != 0)
        {
            return false;
        }
        if (b.size() != marshaledSize)
        {
            return false;
        }

        uint32_t expectedTableSum = 0;
        if (digest_.tab != nullptr)
        {
            std::vector<char> tableData(reinterpret_cast<const char *>(digest_.tab.get()),
                                        reinterpret_cast<const char *>(digest_.tab.get()) + sizeof(Table));
            expectedTableSum = ChecksumIEEE(tableData.data(), tableData.size());
        }

        uint32_t storedTableSum = BEUint32(b, 4);
        if (expectedTableSum != storedTableSum)
        {
            return false;
        }

        digest_.crc = BEUint32(b, 8);

        return true;
    }

    std::shared_ptr<Hash32> CRC32::Clone() const
    {
        return std::make_shared<CRC32>(*this);
    }

    Digest CRC32::GetDigest() const
    {
        return digest_;
    }

    std::shared_ptr<Hash32> New(const TablePtr &tab)
    {
        return std::make_shared<CRC32>(tab);
    }

    std::shared_ptr<Hash32> NewIEEE()
    {
        return std::make_shared<CRC32>(std::shared_ptr<Table>(ieeeTable));
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
