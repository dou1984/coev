#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>

namespace coev::nghttp2
{
    
    class response final
    {
        std::unordered_map<std::string, std::string> m_headers;
        std::string m_status;
        std::string m_body;
        uint32_t m_stream_id;

    public:
        const std::string &status() const;
        const std::string &body() const;
        void add_header(const char *, size_t, const char *, size_t);
        void append(const char *, size_t);
        const std::string &header(const std::string &key);
        void set_stream_id(uint32_t stream_id) { m_stream_id = stream_id; }
      
    };
}