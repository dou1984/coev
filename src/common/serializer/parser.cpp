#include "def.h"
#include "parser.h"

namespace coev::details
{
    parser::parser(const char *m_buf, size_t m_size) : m_buf(m_buf), m_size(m_size)
    {
    }
    size_t parser::parse(int8_t &val)
    {
        if (m_size >= sizeof(uint8_t))
        {
            val = *(int8_t *)m_buf;
            m_buf += sizeof(uint8_t);
            m_size -= sizeof(uint8_t);
            return sizeof(int8_t);
        }
        return 0;
    }
    size_t parser::parse(uint8_t &val)
    {
        if (m_size >= sizeof(uint8_t))
        {
            val = *(uint8_t *)m_buf;
            m_buf += sizeof(uint8_t);
            m_size -= sizeof(uint8_t);
            return sizeof(uint8_t);
        }
        return 0;
    }
    size_t parser::parse(int16_t &val)
    {
        if (m_size >= sizeof(uint16_t))
        {
            val = *(int16_t *)m_buf;
            val = htons(val);
            m_buf += sizeof(uint16_t);
            m_size -= sizeof(uint16_t);
            return sizeof(int16_t);
        }
        return 0;
    }
    size_t parser::parse(uint16_t &val)
    {
        if (m_size >= sizeof(uint16_t))
        {
            val = *(uint16_t *)m_buf;
            val = htons(val);
            m_buf += sizeof(uint16_t);
            m_size -= sizeof(uint16_t);
            return sizeof(uint16_t);
        }
        return 0;
    }
    size_t parser::parse(int32_t &val)
    {
        if (m_size >= sizeof(uint32_t))
        {
            val = *(int32_t *)m_buf;
            val = htonl(val);
            m_buf += sizeof(uint32_t);
            m_size -= sizeof(uint32_t);
            return sizeof(int32_t);
        }
        return 0;
    }
    size_t parser::parse(uint32_t &val)
    {
        if (m_size >= sizeof(uint32_t))
        {
            val = *(uint32_t *)m_buf;
            val = htonl(val);
            m_buf += sizeof(uint32_t);
            m_size -= sizeof(uint32_t);
            return sizeof(uint32_t);
        }
        return 0;
    }
    size_t parser::parse(int64_t &val)
    {
        if (m_size >= sizeof(uint64_t))
        {
            val = *(uint64_t *)m_buf;
            val = htonll(val);
            m_buf += sizeof(uint64_t);
            m_size -= sizeof(uint64_t);
            return sizeof(uint64_t);
        }
        return 0;
    }
    size_t parser::parse(uint64_t &val)
    {
        if (m_size >= sizeof(uint64_t))
        {
            val = *(int64_t *)m_buf;
            val = htonll(val);
            m_buf += sizeof(uint64_t);
            m_size -= sizeof(uint64_t);
            return sizeof(int64_t);
        }
        return 0;
    }

    size_t parser::parse(std::string &m_buf)
    {
        return parse<int16_t, std::string>(m_buf);
    }
    size_t parser::parse_bytes(std::string &m_buf)
    {
        return parse<int32_t, std::string>(m_buf);
    }

    size_t parser::__parse_varint(uint64_t &val)
    {
        val = 0;

        int i = 0;
        auto r = 0;
        assert(m_size != 0);
        do
        {
            val = val | ((m_buf[i] & 0x7f) << (7 * i));
            r = m_buf[i++] & 0x80;
            m_buf++;
            m_size--;
        } while (r == 0);
        return i;
    }
    size_t parser::parse_varint(uint64_t &val)
    {
        return __parse_varint(val);
    }
    size_t parser::parse_varint(int64_t &val)
    {
        uint64_t _val = 0;
        auto r = parse_varint(_val);
        val = int64_t(_val);
        val = (val >> 1) ^ (val << 63);
        return r;
    }
    size_t parser::parse_varint(uint32_t &val)
    {
        uint64_t _val = 0;
        auto r = __parse_varint(_val);
        val = _val;
        return r;
    }
    size_t parser::parse_varint(int32_t &val)
    {
        int64_t _val = 0;
        auto r = parse_varint(_val);
        val = _val;
        return r;
    }
    size_t parser::size() const
    {
        return m_size;
    }

}