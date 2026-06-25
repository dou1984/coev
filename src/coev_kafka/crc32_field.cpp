/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */

#include <mutex>
#include <queue>
#include <arpa/inet.h>
#include <utils/hash/crc32.h>
#include "crc32_field.h"
#include "undefined.h"

namespace coev::kafka
{
    static std::mutex pool_mutex;
    static std::queue<std::shared_ptr<Crc32Field>> crc32_field_pool;

    std::shared_ptr<Crc32Field> acquire_crc32_field(CrcPolynomial polynomial)
    {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if (!crc32_field_pool.empty())
        {
            auto c = crc32_field_pool.front();
            crc32_field_pool.pop();
            c->m_polynomial = polynomial;
            return c;
        }
        return std::make_shared<Crc32Field>(polynomial);
    }

    void release_crc32_field(std::shared_ptr<Crc32Field> c)
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

    Crc32Field::Crc32Field(CrcPolynomial polynomial) : m_start_offset(0), m_polynomial(polynomial) {}

    void Crc32Field::save_offset(int in)
    {
        m_start_offset = in;
    }

    int Crc32Field::reserve_length()
    {
        return 4;
    }

    int Crc32Field::run(int cur_offset, std::string &buf)
    {
        uint32_t crc_val;
        int err = crc(cur_offset, buf, crc_val);
        if (err != ErrNoError)
        {
            LOG_ERR("Crc32Field::run: crc calculation failed with error %d", err);
            return err;
        }
        uint32_t network_crc = htonl(crc_val);
        uint8_t *ptr = reinterpret_cast<uint8_t *>(&network_crc);
        buf[m_start_offset] = ptr[0];
        buf[m_start_offset + 1] = ptr[1];
        buf[m_start_offset + 2] = ptr[2];
        buf[m_start_offset + 3] = ptr[3];
        return 0;
    }

    int Crc32Field::check(int cur_offset, const std::string_view &buf)
    {
        // LOG_CORE("Crc32Field::check start_offset: %d, cur_offset: %d", m_start_offset, cur_offset);
        uint32_t crc_val;
        int err = crc(cur_offset, buf, crc_val);
        if (err != ErrNoError)
        {
            LOG_ERR("Crc32Field::check: crc calculation failed with error %d", err);
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

    int Crc32Field::crc(int cur_offset, const std::string_view &buf, uint32_t &out_crc)
    {
        uint32_t crc = 0xFFFFFFFF;
        if (m_polynomial == CrcCastagnoli)
        {
            for (int i = m_start_offset + 4; i < cur_offset; ++i)
            {
                uint8_t byte = buf[i];
                crc = (crc >> 8) ^ castagnoliTable[(crc & 0xFF) ^ byte];
            }
        }
        else
        {
            for (int i = m_start_offset + 4; i < cur_offset; ++i)
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
}