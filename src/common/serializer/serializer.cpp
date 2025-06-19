#include "def.h"
#include "serializer.h"

namespace coev::details
{

    std::string &&serializer::move()
    {
        return std::move(m_buf);
    }
    size_t serializer::append(bool val)
    {
        m_buf.push_back(!!val);
        return sizeof(bool);
    }
    size_t serializer::append(int8_t val)
    {
        m_buf.push_back((char)val);
        return sizeof(int8_t);
    }
    size_t serializer::append(int16_t val)
    {
        auto _val = htons(val);
        m_buf.append((char *)&_val, sizeof(int16_t));
        return sizeof(int16_t);
    }
    size_t serializer::append(int32_t val)
    {
        auto _val = htonl(val);
        m_buf.append((char *)&_val, sizeof(int32_t));
        return sizeof(int32_t);
    }
    size_t serializer::append(int64_t val)
    {
        auto _val = htonll(val);
        m_buf.append((char *)&_val, sizeof(int64_t));
        return sizeof(int64_t);
    }

    size_t serializer::append_raw(const std::string &val)
    {
        m_buf.append(val);
        return val.size();
    }

    size_t serializer::append(const std::string &val)
    {
        return append<int16_t>(val.c_str(), val.size());
    }
    size_t serializer::append_bytes(const std::string &val)
    {
        return append<int32_t>(val.c_str(), val.size());
    }
    size_t serializer::append_nullable_string(const std::string &val)
    {
        return append_nullable<int16_t, std::string>(val);
    }

    size_t serializer::append_nullable_bytes(const std::string &val)
    {
        return append_nullable<int32_t, std::string>(val);
    }
    size_t serializer::__append_varint(uint64_t val)
    {
        auto l = m_buf.size();
        do
        {
            char c = val & 0x7f;
            val = val >> 7;
            if (val == 0)
            {
                c |= 0x80;
            }
            m_buf.push_back(c);

        } while (val != 0);
        return m_buf.size() - l;
    }
    size_t serializer::append_varint(uint64_t val)
    {
        return __append_varint(val);
    }
    size_t serializer::append_varint(int64_t val)
    {
        return append_varint(uint64_t((val << 1) ^ (val >> 63)));
    }
    size_t serializer::append_varint(uint32_t val)
    {
        return __append_varint(val);
    }
    size_t serializer::append_varint(int32_t val)
    {
        return append_varint(int64_t(val));
    }
}