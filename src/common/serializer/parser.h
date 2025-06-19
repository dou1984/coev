#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
#include <cassert>

namespace coev::details
{
    class parser
    {
    public:
        parser(const char *buf, size_t size);
        size_t parse(int8_t &val);
        size_t parse(uint8_t &val);
        size_t parse(int16_t &val);
        size_t parse(uint16_t &val);
        size_t parse(int32_t &val);
        size_t parse(uint32_t &val);
        size_t parse(int64_t &val);
        size_t parse(uint64_t &val);
        template <typename T, typename V>
        size_t parse(V &val)
        {
            T len;
            if (auto r = parse(len); r != 0)
            {
                if (len != T(-1))
                {
                    if (m_size >= len)
                    {
                        val.assign(m_buf, len);
                        m_buf += len;
                        m_size -= len;
                        return r + len;
                    }
                    else
                    {
                        assert(false);
                    }
                }
                val.clear();
            }
            return 0;
        }
        size_t parse(std::string &buf);
        size_t parse_bytes(std::string &buf);

        size_t parse_varint(uint64_t &val);
        size_t parse_varint(int64_t &val);
        size_t parse_varint(uint32_t &val);
        size_t parse_varint(int32_t &val);
        size_t size() const;

    private:
        size_t __parse_varint(uint64_t &val);
        const char *m_buf = nullptr;
        size_t m_size = 0;
    };
}