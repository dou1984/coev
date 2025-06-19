#pragma once
#include <string>
#include <cstdint>
#include <cstddef>
namespace coev::details
{
    class serializer
    {

    public:
        std::string &&move();
        size_t append(bool val);
        size_t append(int8_t val);
        size_t append(int16_t val);
        size_t append(int32_t val);
        size_t append(int64_t val);
        size_t append_raw(const std::string &val);
        template <class T>
        size_t append(T val)
        {
            return append(val);
        }
        template <typename T>
        size_t append(const char *val, size_t size)
        {
            if constexpr (std::is_same_v<T, int16_t>)
            {
                append<int16_t>(size);
                m_buf.append(val, size);
                return sizeof(int16_t) + size;
            }
            else if constexpr (std::is_same_v<T, int32_t>)
            {
                append<int32_t>(size);
                m_buf.append(val, size);
                return sizeof(int32_t) + size;
            }
        }
        size_t append(const std::string &val);
        size_t append_bytes(const std::string &val);

        template <typename T, typename V>
        size_t append_nullable(const V &val)
        {
            return val.empty() ? append(T(-1)) : append(val);
        }
        size_t append_nullable_string(const std::string &val);
        size_t append_nullable_bytes(const std::string &val);

        size_t append_varint(uint64_t val);
        size_t append_varint(int64_t val);
        size_t append_varint(uint32_t val);
        size_t append_varint(int32_t val);

    private:
        size_t __append_varint(uint64_t val);

    private:
        std::string m_buf;
    };

}