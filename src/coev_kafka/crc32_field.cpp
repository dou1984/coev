
#include <mutex>
#include <queue>
#include <arpa/inet.h>
#include <utils/hash/crc32.h>
#include "crc32_field.h"
#include "undefined.h"

static std::mutex pool_mutex;
static std::queue<std::shared_ptr<crc32_field>> crc32_field_pool;

std::shared_ptr<crc32_field> acquire_crc32_field(CrcPolynomial polynomial)
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    if (!crc32_field_pool.empty())
    {
        auto c = crc32_field_pool.front();
        crc32_field_pool.pop();
        c->m_polynomial = polynomial;
        return c;
    }
    return std::make_shared<crc32_field>(polynomial);
}

void release_crc32_field(std::shared_ptr<crc32_field> c)
{
    std::lock_guard<std::mutex> lock(pool_mutex);
    crc32_field_pool.push(c);
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

crc32_field::crc32_field(CrcPolynomial polynomial) : m_start_offset(0), m_polynomial(polynomial) {}

void crc32_field::save_offset(int in)
{
    LOG_CORE("crc32_field::save_offset startOffset: %d", in);
    m_start_offset = in;
}

int crc32_field::reserve_length()
{
    return 4;
}

int crc32_field::run(int curOffset, std::string &buf)
{
    // LOG_CORE("crc32_field::run startOffset: %d, curOffset: %d", startOffset, curOffset);
    uint32_t crc_val;
    int err = crc(curOffset, buf, crc_val);
    if (err != ErrNoError)
    {
        LOG_ERR("crc32_field::run: crc calculation failed with error %d", err);
        return err;
    }
    // LOG_CORE("crc32_field::run calculated CRC value: 0x%08x", crc_val);
    uint32_t network_crc = htonl(crc_val);
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&network_crc);
    buf[m_start_offset] = ptr[0];
    buf[m_start_offset + 1] = ptr[1];
    buf[m_start_offset + 2] = ptr[2];
    buf[m_start_offset + 3] = ptr[3];
    // LOG_CORE("crc32_field::run CRC value written to buffer");
    return 0;
}

int crc32_field::check(int curOffset, const std::string_view &buf)
{
    LOG_CORE("crc32_field::check startOffset: %d, curOffset: %d", m_start_offset, curOffset);
    uint32_t crc_val;
    int err = crc(curOffset, buf, crc_val);
    if (err != ErrNoError)
    {
        LOG_ERR("crc32_field::check: crc calculation failed with error %d", err);
        return err;
    }

    uint32_t network_expected;
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&network_expected);
    ptr[0] = buf[m_start_offset];
    ptr[1] = buf[m_start_offset + 1];
    ptr[2] = buf[m_start_offset + 2];
    ptr[3] = buf[m_start_offset + 3];
    uint32_t expected = ntohl(network_expected);

    if (crc_val != expected)
    {
        return ErrCRCMismatch;
    }

    return ErrNoError;
}

int crc32_field::crc(int curOffset, const std::string_view &buf, uint32_t &out_crc)
{
    uint32_t crc = 0xFFFFFFFF;
    if (m_polynomial == CrcCastagnoli)
    {
        // Use the precomputed Castagnoli table for faster calculation
        for (int i = m_start_offset + 4; i < curOffset; ++i)
        {
            uint8_t byte = buf[i];
            crc = (crc >> 8) ^ castagnoliTable[(crc & 0xFF) ^ byte];
        }
    }
    else
    {
        // Use the standard IEEE polynomial calculation
        for (int i = m_start_offset + 4; i < curOffset; ++i)
        {
            uint8_t byte = buf[i];
            crc ^= static_cast<uint32_t>(byte);
            for (int j = 0; j < 8; ++j)
            {
                if (crc & 1)
                {
                    crc = (crc >> 1) ^ 0xEDB88320;
                }
                else
                {
                    crc >>= 1;
                }
            }
        }
    }
    out_crc = crc ^ 0xFFFFFFFF;
    return ErrNoError;
}