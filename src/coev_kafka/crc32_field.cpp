
#include <mutex>
#include <queue>
#include <zlib.h>
#include "crc32_field.h"
#include "undefined.h"

static std::mutex pool_mutex;
static std::queue<std::shared_ptr<crc32Field>> crc32FieldPool;

std::shared_ptr<crc32Field> acquireCrc32Field(CrcPolynomial polynomial)
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    if (!crc32FieldPool.empty())
    {
        auto c = crc32FieldPool.front();
        crc32FieldPool.pop();
        c->polynomial = polynomial;
        return c;
    }
    return std::make_shared<crc32Field>(polynomial);
}

void releaseCrc32Field(std::shared_ptr<crc32Field> c)
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    crc32FieldPool.push(c);
}

static const uint32_t *castagnoliTable = []()
{
    static uint32_t table[256];
    for (int i = 0; i < 256; ++i)
    {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0x82F63B78;
            }
            else
            {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}();

crc32Field::crc32Field(CrcPolynomial polynomial)
    : startOffset(0), polynomial(polynomial) {}

void crc32Field::saveOffset(int in)
{
    startOffset = in;
}

int crc32Field::reserveLength()
{
    return 4;
}

int crc32Field::run(int curOffset, std::string &buf)
{
    uint32_t crc_val;
    int err = crc(curOffset, buf, crc_val);
    if (err != ErrNoError)
    {
        return err;
    }
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&crc_val);
    buf[startOffset] = ptr[3];
    buf[startOffset + 1] = ptr[2];
    buf[startOffset + 2] = ptr[1];
    buf[startOffset + 3] = ptr[0];
    return 0;
}

int crc32Field::check(int curOffset, std::string &buf)
{
    uint32_t crc_val;
    int err = crc(curOffset, buf, crc_val);
    if (err != ErrNoError)
    {
        return err;
    }

    uint32_t expected = (static_cast<uint32_t>(buf[startOffset]) << 24) |
                        (static_cast<uint32_t>(buf[startOffset + 1]) << 16) |
                        (static_cast<uint32_t>(buf[startOffset + 2]) << 8) |
                        static_cast<uint32_t>(buf[startOffset + 3]);

    if (crc_val != expected)
    {
        return ErrCRCMismatch;
    }

    return ErrNoError;
}

int crc32Field::crc(int curOffset, const std::string &buf, uint32_t &out_crc)
{
    const uint32_t *tab = nullptr;
    switch (polynomial)
    {
    case CrcPolynomial::CrcIEEE:
        tab = crc32_ieee_table();
        break;
    case CrcPolynomial::CrcCastagnoli:
        tab = castagnoliTable;
        break;
    default:
        return ErrInvalidCRCTYPE;
    }

    uint32_t crc = 0xFFFFFFFF;
    for (int i = startOffset + 4; i < curOffset; ++i)
    {
        uint8_t byte = buf[i];
        crc = tab[(crc ^ byte) & 0xFF] ^ (crc >> 8);
    }
    out_crc = crc ^ 0xFFFFFFFF;
    return ErrNoError;
}