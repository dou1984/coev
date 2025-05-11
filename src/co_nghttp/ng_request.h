#pragma once
#include <string>
#include <unordered_map>
#include <nghttp2/nghttp2.h>
#include "ng_header.h"

namespace coev::nghttp2
{
    class ng_request final
    {
        std::unordered_map<std::string, std::string> m_headers;
        std::string m_body;
        int32_t m_stream_id;

    public:  
        void add_header(const char *, size_t, const char *, size_t);   
        void set_stream_id(int32_t);
        void append(const char *, size_t);
        const std::string &header(const std::string &);
        const std::string &path();
        const std::string &body();        
        int32_t id() const;
    };
}